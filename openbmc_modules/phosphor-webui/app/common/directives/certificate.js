window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('certificate', [
    'APIUtils',
    function(APIUtils) {
      return {
        'restrict': 'E',
        'template': require('./certificate.html'),
        'scope': {'cert': '=', 'reload': '&'},
        'controller': [
          '$scope', 'APIUtils', 'toastService', 'Constants', '$uibModal',
          function($scope, APIUtils, toastService, Constants, $uibModal) {
            var certificateType = 'PEM';
            var availableCertificateTypes = Constants.CERTIFICATE_TYPES;

            /**
             * This function is needed to map the backend Description to what
             * should appear in the GUI. This is needed specifically for CA
             * certificate types. The backend description for the certificate
             * type is 'TrustStore Certificate', this function will make sure we
             * display 'CA Certificate' on the frontend
             * @param {string} : certificate Description property
             * @returns {string} : certificate name that should appear on GUI
             */
            $scope.getCertificateName = function(certificateDescription) {
              var matched =
                  availableCertificateTypes.find(function(certificate) {
                    return certificate.Description === certificateDescription;
                  });
              if (matched === undefined) {
                return '';
              } else {
                return matched.name;
              }
            };

            $scope.isDeletable = function(certificate) {
              return certificate.Description == 'TrustStore Certificate';
            };

            $scope.confirmDeleteCert = function(certificate) {
              initRemoveModal(certificate);
            };

            /**
             * Intiate remove certificate modal
             * @param {*} certificate
             */
            function initRemoveModal(certificate) {
              const template = require('./certificate-modal-remove.html');
              $uibModal
                  .open({
                    template,
                    windowTopClass: 'uib-modal',
                    ariaLabelledBy: 'dialog_label',
                    controllerAs: 'modalCtrl',
                    controller: function() {
                      this.certificate = certificate;
                    }
                  })
                  .result
                  .then(() => {
                    deleteCert(certificate);
                  })
                  .catch(
                      () => {
                          // do nothing
                      })
            };

            /**
             * Removes certificate
             * @param {*} certificate
             */
            function deleteCert(certificate) {
              $scope.confirm_delete = false;
              APIUtils.deleteRedfishObject(certificate['@odata.id'])
                  .then(
                      function(data) {
                        $scope.loading = false;
                        toastService.success(
                            $scope.getCertificateName(certificate.Description) +
                            ' was deleted.');
                        $scope.reload();
                      },
                      function(error) {
                        console.log(error);
                        $scope.loading = false;
                        toastService.error(
                            'Unable to delete ' +
                            $scope.getCertificateName(certificate.Description));
                      });
              return;
            };

            $scope.replaceCertificate = function(certificate) {
              $scope.loading = true;
              if (certificate.file.name.split('.').pop() !==
                  certificateType.toLowerCase()) {
                toastService.error(
                    'Certificate must be replaced with a .pem file.');
                return;
              }
              var file =
                  document
                      .getElementById(
                          'upload_' + certificate.Description + certificate.Id)
                      .files[0];
              var reader = new FileReader();
              reader.onloadend = function(e) {
                var data = {};
                data.CertificateString = e.target.result;
                data.CertificateUri = {'@odata.id': certificate['@odata.id']};
                data.CertificateType = certificateType;
                APIUtils.replaceCertificate(data).then(
                    function(data) {
                      $scope.loading = false;
                      toastService.success(
                          $scope.getCertificateName(certificate.Description) +
                          ' was replaced.');
                      $scope.reload();
                    },
                    function(error) {
                      console.log(error);
                      $scope.loading = false;
                      toastService.error(
                          'Unable to replace ' +
                          $scope.getCertificateName(certificate.Description));
                    });
              };
              reader.readAsBinaryString(file);
            };
          }
        ]
      };
    }
  ]);
})(window.angular);
