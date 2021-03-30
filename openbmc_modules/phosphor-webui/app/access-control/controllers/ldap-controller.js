/**
 * Controller for LDAP
 *
 * @module app/access-control
 * @exports ldapController
 * @name ldapController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.accessControl').controller('ldapController', [
    '$scope', 'APIUtils', '$q', 'toastService',
    function($scope, APIUtils, $q, toastService) {
      $scope.loading = false;
      $scope.isSecure = false;
      $scope.ldapProperties = {};
      $scope.originalLdapProperties = {};
      $scope.submitted = false;
      $scope.roleGroups = [];
      $scope.roleGroupType = '';
      $scope.ldapCertExpiration = '';
      $scope.caCertExpiration = '';

      $scope.$on('$viewContentLoaded', function() {
        $scope.loadLdap();
      });

      $scope.loadLdap = function() {
        $scope.loading = true;
        $scope.submitted = false;
        const ldapAccountProperties =
            APIUtils.getAllUserAccountProperties().then(
                function(data) {
                  const serviceEnabled = data.LDAP.ServiceEnabled ||
                      data.ActiveDirectory.ServiceEnabled;
                  const ldapServiceEnabled = data.LDAP.ServiceEnabled;
                  const adServiceEnabled = data.ActiveDirectory.ServiceEnabled;
                  const enabledServiceType = getEnabledServiceType(data);
                  const serviceAddresses = getServiceAddresses(data);
                  const useSSL = getUseSsl(data);
                  const userName = getUsername(data);
                  const baseDistinguishedNames =
                      getBaseDistinguishedNames(data);
                  const groupsAttribute = getGroupsAttribute(data);
                  const usernameAttribute = getUsernameAttribute(data);
                  const roleGroups = getRoleGroups(data);


                  return {
                    'ServiceEnabled': serviceEnabled,
                    'LDAPServiceEnabled': ldapServiceEnabled,
                    'ADServiceEnabled': adServiceEnabled,
                    'EnabledServiceType': enabledServiceType,
                    'ServiceAddresses': serviceAddresses,
                    'useSSL': useSSL,
                    'Username': userName,
                    'Password': null,
                    'BaseDistinguishedNames': baseDistinguishedNames,
                    'GroupsAttribute': groupsAttribute,
                    'UsernameAttribute': usernameAttribute,
                    'RoleGroups': roleGroups
                  };
                },
                function(error) {
                  console.log(JSON.stringify(error));
                });

        const ldapCertificate =
            getCertificate('/redfish/v1/AccountService/LDAP/Certificates');

        const caCertificate =
            getCertificate('/redfish/v1/Managers/bmc/Truststore/Certificates/');

        const promises =
            [ldapAccountProperties, ldapCertificate, caCertificate];
        $q.all(promises).then(function(results) {
          $scope.ldapProperties = results[0];
          $scope.originalLdapProperties = angular.copy(results[0]);
          $scope.roleGroupType = results[0].EnabledServiceType;
          $scope.roleGroups = results[0].RoleGroups;
          $scope.ldapCertificate = results[1];
          $scope.caCertificate = results[2];
          $scope.loading = false;
        });
      };

      /**
       * Save LDAP settings
       * Will be making two calls every time to accomodate the backend design
       * LDAP and ActiveDirectory changes can not be sent together when changing
       * from ActiveDirectory to LDAP
       */
      $scope.saveLdapSettings = function() {
        const enabledServiceType = $scope.ldapProperties.EnabledServiceType;
        const enabledServicePayload =
            createLdapEnableRequest(enabledServiceType, $scope.ldapProperties);
        const disabledServiceType =
            enabledServiceType == 'LDAP' ? 'ActiveDirectory' : 'LDAP';
        const disabledServicePayload =
            createLdapDisableRequest(disabledServiceType);

        APIUtils.saveLdapProperties(disabledServicePayload)
            .then(function(response) {
              return APIUtils.saveLdapProperties(enabledServicePayload);
            })
            .then(
                function(response) {
                  if (!response.data.hasOwnProperty('error')) {
                    toastService.success('Successfully updated LDAP settings.');
                    $scope.loadLdap();
                  } else {
                    // The response returned a 200 but there was an error
                    // property sent in the response. It is unclear what
                    // settings were saved. Reloading LDAP to make it clear
                    // to the user.
                    toastService.error('Unable to update all LDAP settings.');
                    $scope.loadLdap();
                    console.log(response.data.error.message);
                  }
                },
                function(error) {
                  toastService.error('Unable to update LDAP settings.');
                  console.log(JSON.stringify(error));
                });
      };

      /**
       * Sends a request to disable the LDAP Service if the user
       * toggled the service enabled checkbox in the UI and if
       * there was a previously saved service type. This prevents
       * unnecessary calls to the backend if the user toggled
       * the service enabled, but never actually had saved settings.
       */
      $scope.updateServiceEnabled = () => {
        const originalEnabledServiceType =
            $scope.originalLdapProperties.EnabledServiceType;

        if (!$scope.ldapProperties.ServiceEnabled &&
            originalEnabledServiceType) {
          $scope.ldapProperties.EnabledServiceType = '';

          const disabledServicePayload =
              createLdapDisableRequest(originalEnabledServiceType);
          APIUtils.saveLdapProperties(disabledServicePayload)
              .then(
                  function(response) {
                    toastService.success('Successfully disabled LDAP.');
                    $scope.roleGroups = [];
                    $scope.loadLdap();
                  },
                  function(error) {
                    toastService.error('Unable to update LDAP settings.');
                    console.log(JSON.stringify(error));
                  });
        }
      };

      /**
       *
       * @param {string} uri for certificate
       * @returns {null | Object}
       */
      function getCertificate(location) {
        return APIUtils.getCertificate(location).then(function(data) {
          if (data.Members && data.Members.length) {
            return APIUtils.getCertificate(data.Members[0]['@odata.id'])
                .then(
                    function(response) {
                      return response;
                    },
                    function(error) {
                      console.log(json.stringify(error));
                    })
          } else {
            return null;
          }
        });
      }

      /**
       *
       * @param {Object} ldapProperties
       * @returns {string}
       */
      function getEnabledServiceType(ldapProperties) {
        let enabledServiceType = '';
        if (ldapProperties.LDAP.ServiceEnabled) {
          enabledServiceType = 'LDAP';
        } else if (ldapProperties.ActiveDirectory.ServiceEnabled) {
          enabledServiceType = 'ActiveDirectory';
        }
        return enabledServiceType;
      }

      /**
       *
       * @param {Object} ldapProperties
       * @returns {Array}
       */
      function getServiceAddresses(ldapProperties) {
        let serviceAddresses = [];
        let serviceType = getEnabledServiceType(ldapProperties);
        if (serviceType) {
          serviceAddresses = ldapProperties[serviceType]['ServiceAddresses'];
        }
        return serviceAddresses;
      }

      /**
       *
       * @param {Object} ldapProperties
       * @returns {boolean}
       */
      function getUseSsl(ldapProperties) {
        let useSsl = false;
        let serviceType = getEnabledServiceType(ldapProperties);
        if (serviceType) {
          const uri = ldapProperties[serviceType]['ServiceAddresses'][0];
          useSsl = uri.startsWith('ldaps://');
        }
        return useSsl;
      }

      /**
       *
       * @param {Object} ldapProperties
       * @returns {string}
       */
      function getUsername(ldapProperties) {
        let username = '';
        let serviceType = getEnabledServiceType(ldapProperties);
        if (serviceType) {
          username = ldapProperties[serviceType]['Authentication']['Username'];
        }
        return username;
      }

      /**
       *
       * @param {Object} ldapProperties
       * @returns {Array}
       */
      function getBaseDistinguishedNames(ldapProperties) {
        let basedDisguishedNames = [];
        let serviceType = getEnabledServiceType(ldapProperties);
        if (serviceType) {
          basedDisguishedNames =
              ldapProperties[serviceType]['LDAPService']['SearchSettings']['BaseDistinguishedNames'];
        }
        return basedDisguishedNames;
      }

      /**
       *
       * @param {Object} ldapProperties
       * @returns {string}
       */
      function getGroupsAttribute(ldapProperties) {
        let groupsAttribute = '';
        let serviceType = getEnabledServiceType(ldapProperties);
        if (serviceType) {
          groupsAttribute =
              ldapProperties[serviceType]['LDAPService']['SearchSettings']['GroupsAttribute'];
        }
        return groupsAttribute;
      }

      /**
       *
       * @param {Object} ldapProperties
       * @returns {string}
       */
      function getUsernameAttribute(ldapProperties) {
        let userNameAttribute = '';
        let serviceType = getEnabledServiceType(ldapProperties);
        if (serviceType) {
          userNameAttribute =
              ldapProperties[serviceType]['LDAPService']['SearchSettings']['UsernameAttribute'];
        }
        return userNameAttribute;
      }

      /**
       *
       * @param {Object} ldapProperties
       * @returns {Array} A list of role groups
       */
      function getRoleGroups(ldapProperties) {
        let roleGroups = [];
        let serviceType = getEnabledServiceType(ldapProperties);
        if (serviceType) {
          roleGroups = ldapProperties[serviceType]['RemoteRoleMapping'];
        }

        return roleGroups;
      }

      /**
       * Returns the payload needed to enable an LDAP Service
       * @param {string} serviceType - 'LDAP' or 'ActiveDirectory'
       */
      function createLdapEnableRequest(serviceType, ldapProperties) {
        let ldapRequest = {};
        const ServiceEnabled = true;
        const Authentication = {
          Username: ldapProperties.Username,
          Password: ldapProperties.Password
        };
        const LDAPService = {
          SearchSettings: {
            BaseDistinguishedNames: ldapProperties.BaseDistinguishedNames,
            GroupsAttribute: ldapProperties.GroupsAttribute,
            UsernameAttribute: ldapProperties.UsernameAttribute
          }
        };
        const ServiceAddresses = ldapProperties.ServiceAddresses;

        if (serviceType == 'LDAP') {
          ldapRequest = {
            LDAP:
                {ServiceEnabled, Authentication, LDAPService, ServiceAddresses}
          };
        } else {
          ldapRequest = {
            ActiveDirectory:
                {ServiceEnabled, Authentication, LDAPService, ServiceAddresses}
          };
        }
        return ldapRequest;
      };

      /**
       * Returns the payload needed to disable an LDAP Service
       * @param {string} serviceType - 'LDAP' or 'ActiveDirectory'
       */
      function createLdapDisableRequest(serviceType) {
        let ldapRequest = {};
        const ServiceEnabled = false;

        if (serviceType == 'LDAP') {
          ldapRequest = {LDAP: {ServiceEnabled}};
        } else {
          ldapRequest = {ActiveDirectory: {ServiceEnabled}};
        }
        return ldapRequest;
      }
    }
  ]);
})(angular);
