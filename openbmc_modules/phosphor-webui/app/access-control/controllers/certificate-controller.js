/**
 * Controller for Certificate Management
 *
 * @module app/access-control
 * @exports certificateController
 * @name certificateController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.configuration').controller('certificateController', [
    '$scope', 'APIUtils', '$q', 'Constants', 'toastService', '$timeout',
    '$uibModal',
    function(
        $scope, APIUtils, $q, Constants, toastService, $timeout, $uibModal) {
      $scope.loading = false;
      $scope.certificates = [];
      $scope.availableCertificateTypes = [];
      $scope.allCertificateTypes = Constants.CERTIFICATE_TYPES;
      $scope.newCertificate = {};
      $scope.newCSR = {};
      $scope.keyBitLength = Constants.CERTIFICATE.KEY_BIT_LENGTH;
      $scope.keyPairAlgorithm = Constants.CERTIFICATE.KEY_PAIR_ALGORITHM;
      $scope.keyCurveId = Constants.CERTIFICATE.KEY_CURVE_ID;
      $scope.countryList = Constants.COUNTRIES;

      $scope.$on('$viewContentLoaded', () => {
        getBmcTime();
      })

      $scope.loadCertificates = function() {
        $scope.certificates = [];
        $scope.availableCertificateTypes = Constants.CERTIFICATE_TYPES;
        $scope.loading = true;
        // Use Certificate Service to get the locations of all the certificates,
        // then add a promise for fetching each certificate
        APIUtils.getCertificateLocations().then(
            function(data) {
              var promises = [];
              var locations = data.Links.Certificates;
              for (var i in locations) {
                var location = locations[i];
                promises.push(getCertificatePromise(location['@odata.id']));
              }
              $q.all(promises)
                  .catch(function(error) {
                    toastService.error('Failed to load certificates.');
                    console.log(JSON.stringify(error));
                  })
                  .finally(function() {
                    $scope.loading = false;
                    $scope.certificates.sort(function(a, b) {
                      if (a.Name > b.Name) {
                        return 1;
                      }
                      if (a.Name < b.Name) {
                        return -1;
                      }
                      if (a.Issuer.CommonName > b.Issuer.CommonName) {
                        return 1;
                      }
                      if (a.Issuer.CommonName < b.Issuer.CommonName) {
                        return -1;
                      }
                      return (Date.parse(a.ValidNotBefore) >
                              Date.parse(b.ValidNotBefore)) ?
                          1 :
                          -1;
                    });
                  });
            },
            function(error) {
              $scope.loading = false;
              $scope.availableCertificateTypes = [];
              toastService.error('Failed to load certificates.');
              console.log(JSON.stringify(error));
            });
      };

      $scope.uploadCertificate = function() {
        if ($scope.newCertificate.file.name.split('.').pop() !== 'pem') {
          toastService.error('Certificate must be a .pem file.');
          return;
        }
        APIUtils
            .addNewCertificate(
                $scope.newCertificate.file, $scope.newCertificate.selectedType)
            .then(
                function(data) {
                  toastService.success(
                      $scope.newCertificate.selectedType.name +
                      ' was uploaded.');
                  $scope.newCertificate = {};
                  $scope.loadCertificates();
                },
                function(error) {
                  toastService.error(
                      $scope.newCertificate.selectedType.name +
                      ' failed upload.');
                  console.log(JSON.stringify(error));
                });
      };

      var getCertificatePromise = function(url) {
        var promise = APIUtils.getCertificate(url).then(function(data) {
          var certificate = data;
          isExpiring(certificate);
          updateAvailableTypes(certificate);
          $scope.certificates.push(certificate);
        });
        return promise;
      };

      var isExpiring = function(certificate) {
        // convert certificate time to epoch time
        // if ValidNotAfter is less than or equal to 30 days from bmc time
        // (2592000000), isExpiring. If less than or equal to 0, is expired.
        // dividing bmc time by 1000 converts epoch milliseconds to seconds
        var difference = (new Date(certificate.ValidNotAfter).getTime()) -
            ($scope.bmcTime) / 1000;
        if (difference <= 0) {
          certificate.isExpired = true;
        } else if (difference <= 2592000000) {
          certificate.isExpiring = true;
        } else {
          certificate.isExpired = false;
          certificate.isExpiring = false;
        }
      };

      // add optional name
      $scope.names = [];
      $scope.addOptionalRow = function() {
        $scope.names.push({Value: ''})
      };

      // remove optional name row
      $scope.deleteOptionalRow = function(index) {
        $scope.names.splice(index, 1);
        if ($scope.names.length == 0) {
          $scope.names = [];
        }
      };

      // create a CSR object to send to the backend
      $scope.getCSRCode = function() {
        var addCSR = {};
        let alternativeNames = $scope.names.map(name => name.Value);

        // if user provided a first alternative name then push to alternative
        // names array
        $scope.newCSR.firstAlternativeName ?
            alternativeNames.push($scope.newCSR.firstAlternativeName) :
            $scope.newCSR.firstAlternativeName = '';

        addCSR.CertificateCollection = {
          '@odata.id': $scope.newCSR.certificateCollection.location
        };
        addCSR.CommonName = $scope.newCSR.commonName;
        addCSR.ContactPerson = $scope.newCSR.contactPerson || '';
        addCSR.City = $scope.newCSR.city;
        addCSR.AlternativeNames = alternativeNames || [];
        addCSR.ChallengePassword = $scope.newCSR.challengePassword || '';
        addCSR.Email = $scope.newCSR.emailAddress || '';
        addCSR.Country = $scope.newCSR.countryCode.code;
        addCSR.Organization = $scope.newCSR.organization;
        addCSR.OrganizationalUnit = $scope.newCSR.companyUnit;
        addCSR.KeyCurveId = $scope.newCSR.keyCurveId || '';
        addCSR.KeyBitLength = $scope.newCSR.keyBitLength
        addCSR.KeyPairAlgorithm = $scope.newCSR.keyPairAlgorithm || '';
        addCSR.State = $scope.newCSR.state;

        APIUtils.createCSRCertificate(addCSR).then(
            function(data) {
              $scope.csrCode = data;
              openDownloadCsrModal();
            },
            function(error) {
              toastService.error('Unable to generate CSR. Try again.');
              console.log(JSON.stringify(error));
            })
      };

      function openDownloadCsrModal() {
        const modalTemplateCsrDownload =
            require('./certificate-modal-csr-download.html');
        $uibModal
            .open({
              template: modalTemplateCsrDownload,
              windowTopClass: 'uib-modal',
              scope: $scope,
              ariaLabelledBy: 'modal_label',
              size: 'lg',
            })
            .result.catch(function() {
              resetCSRModal();
            });
      };

      $scope.addCertModal = function() {
        openAddCertModal();
      };

      function openAddCertModal() {
        const modalTemplateAddCert =
            require('./certificate-modal-add-cert.html');
        $uibModal
            .open({
              template: modalTemplateAddCert,
              windowTopClass: 'uib-modal',
              scope: $scope,
              ariaLabelledBy: 'modal_label',
            })
            .result.catch(function() {
              // do nothing
            });
      };

      $scope.addCsrModal = function() {
        openCsrModal();
      };

      function openCsrModal() {
        const modalTemplateCsrGen = require('./certificate-modal-csr-gen.html');
        $uibModal
            .open({
              template: modalTemplateCsrGen,
              windowTopClass: 'uib-modal',
              scope: $scope,
              ariaLabelledBy: 'modal_label',
              size: 'lg',
            })
            .result.catch(function() {
              resetCSRModal();
            });
      };

      // resetting the modal when user clicks cancel/closes the
      // modal
      const resetCSRModal = function() {
        $scope.newCSR.certificateCollection = $scope.selectOption;
        $scope.newCSR.commonName = '';
        $scope.newCSR.contactPerson = '';
        $scope.newCSR.city = '';
        $scope.names = [];
        $scope.newCSR.challengePassword = '';
        $scope.newCSR.emailAddress = '';
        $scope.newCSR.countryCode = '';
        $scope.newCSR.keyCurveId = '';
        $scope.newCSR.firstAlternativeName = '';
        $scope.newCSR.keyBitLength = $scope.selectOption;
        $scope.newCSR.keyPairAlgorithm = $scope.selectOption;
        $scope.newCSR.organization = '';
        $scope.newCSR.companyUnit = '';
        $scope.newCSR.state = '';
      };

      // copies the CSR code
      $scope.copySuccess = function(event) {
        $scope.copied = true;
        $timeout(function() {
          $scope.copied = false;
        }, 5000);
      };
      $scope.copyFailed = function(err) {
        console.log(JSON.stringify(err));
      };


      var getBmcTime = function() {
        APIUtils.getBMCTime().then(function(data) {
          $scope.bmcTime = data.data.Elapsed;
        });

        return $scope.bmcTime;
      };

      var updateAvailableTypes = function(certificate) {
        $scope.availableCertificateTypes =
            $scope.availableCertificateTypes.filter(function(type) {
              if (type.Description == 'TrustStore Certificate') {
                return true;
              }
              return type.Description !== certificate.Description;
            });
      };

      $scope.getDays = function(endDate) {
        // finds number of days until certificate expiration
        // dividing bmc time by 1000 converts milliseconds to seconds
        var ms = (new Date(endDate).getTime()) - ($scope.bmcTime) / 1000;
        return Math.floor(ms / (24 * 60 * 60 * 1000));
      };

      $scope.loadCertificates();
    }
  ]);
})(angular);
