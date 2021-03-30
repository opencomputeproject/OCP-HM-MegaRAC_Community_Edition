/**
 * Controller for power-operations
 *
 * @module app/serverControl
 * @exports powerOperationsController
 * @name powerOperationsController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.serverControl').controller('powerOperationsController', [
    '$scope', 'APIUtils', 'dataService', 'Constants', '$interval', '$q',
    'toastService', '$uibModal',
    function(
        $scope, APIUtils, dataService, Constants, $interval, $q, toastService,
        $uibModal) {
      $scope.dataService = dataService;
      $scope.loading = true;
      $scope.oneTimeBootEnabled = false;
      $scope.bootOverrideError = false;
      $scope.bootSources = [];
      $scope.boot = {};
      $scope.defaultRebootSetting = 'warm-reboot';
      $scope.defaultShutdownSetting = 'warm-shutdown';

      $scope.activeModal;

      // When a power operation is in progress, set to true,
      // when a power operation completes (success/fail) set to false.
      // This property is used to show/hide the 'in progress' message
      // in markup.
      $scope.operationPending = false;

      const modalTemplate = require('./power-operations-modal.html');

      const powerOperations =
          {WARM_REBOOT: 0, COLD_REBOOT: 1, WARM_SHUTDOWN: 2, COLD_SHUTDOWN: 3};

      /**
       * Checks the host status provided by the dataService using an
       * interval timer
       * @param {string} statusType : host status type to check for
       * @param {number} timeout : timeout limit, defaults to 5 minutes
       * @param {string} error : error message, defaults to 'Time out'
       * @returns {Promise} : returns a deferred promise that will be fulfilled
       * if the status is met or be rejected if the timeout is reached
       */
      const checkHostStatus =
          (statusType, timeout = 300000, error = 'Time out.') => {
            const deferred = $q.defer();
            const start = new Date();
            const checkHostStatusInverval = $interval(() => {
              let now = new Date();
              let timePassed = now.getTime() - start.getTime();
              if (timePassed > timeout) {
                deferred.reject(error);
                $interval.cancel(checkHostStatusInverval);
              }
              if (dataService.server_state === statusType) {
                deferred.resolve();
                $interval.cancel(checkHostStatusInverval);
              }
            }, Constants.POLL_INTERVALS.POWER_OP);
            return deferred.promise;
          };

      /**
       * Initiate Orderly reboot
       * Attempts to stop all software
       */
      const warmReboot = () => {
        $scope.operationPending = true;
        dataService.setUnreachableState();
        APIUtils.hostReboot()
            .then(() => {
              // Check for off state
              return checkHostStatus(
                  Constants.HOST_STATE_TEXT.off, Constants.TIMEOUT.HOST_OFF,
                  Constants.MESSAGES.POLL.HOST_OFF_TIMEOUT);
            })
            .then(() => {
              // Check for on state
              return checkHostStatus(
                  Constants.HOST_STATE_TEXT.on, Constants.TIMEOUT.HOST_ON,
                  Constants.MESSAGES.POLL.HOST_ON_TIMEOUT);
            })
            .catch(error => {
              console.log(error);
              toastService.error(
                  Constants.MESSAGES.POWER_OP.WARM_REBOOT_FAILED);
            })
            .finally(() => {
              $scope.operationPending = false;
            });
      };

      /**
       * Initiate Immediate reboot
       * Does not attempt to stop all software
       */
      const coldReboot = () => {
        $scope.operationPending = true;
        dataService.setUnreachableState();
        APIUtils.chassisPowerOff()
            .then(() => {
              // Check for off state
              return checkHostStatus(
                  Constants.HOST_STATE_TEXT.off,
                  Constants.TIMEOUT.HOST_OFF_IMMEDIATE,
                  Constants.MESSAGES.POLL.HOST_OFF_TIMEOUT);
            })
            .then(() => {
              return APIUtils.hostPowerOn();
            })
            .then(() => {
              // Check for on state
              return checkHostStatus(
                  Constants.HOST_STATE_TEXT.on, Constants.TIMEOUT.HOST_ON,
                  Constants.MESSAGES.POLL.HOST_ON_TIMEOUT);
            })
            .catch(error => {
              console.log(error);
              toastService.error(
                  Constants.MESSAGES.POWER_OP.COLD_REBOOT_FAILED);
            })
            .finally(() => {
              $scope.operationPending = false;
            });
      };

      /**
       * Initiate Orderly shutdown
       * Attempts to stop all software
       */
      const orderlyShutdown = () => {
        $scope.operationPending = true;
        dataService.setUnreachableState();
        APIUtils.hostPowerOff()
            .then(() => {
              // Check for off state
              return checkHostStatus(
                  Constants.HOST_STATE_TEXT.off, Constants.TIMEOUT.HOST_OFF,
                  Constants.MESSAGES.POLL.HOST_OFF_TIMEOUT);
            })
            .catch(error => {
              console.log(error);
              toastService.error(
                  Constants.MESSAGES.POWER_OP.ORDERLY_SHUTDOWN_FAILED);
            })
            .finally(() => {
              $scope.operationPending = false;
            });
      };

      /**
       * Initiate Immediate shutdown
       * Does not attempt to stop all software
       */
      const immediateShutdown = () => {
        $scope.operationPending = true;
        dataService.setUnreachableState();
        APIUtils.chassisPowerOff()
            .then(() => {
              // Check for off state
              return checkHostStatus(
                  Constants.HOST_STATE_TEXT.off,
                  Constants.TIMEOUT.HOST_OFF_IMMEDIATE,
                  Constants.MESSAGES.POLL.HOST_OFF_TIMEOUT);
            })
            .then(() => {
              dataService.setPowerOffState();
            })
            .catch(error => {
              console.log(error);
              toastService.error(
                  Constants.MESSAGES.POWER_OP.IMMEDIATE_SHUTDOWN_FAILED);
            })
            .finally(() => {
              $scope.operationPending = false;
            });
      };

      /**
       * Initiate Power on
       */
      $scope.powerOn = () => {
        $scope.operationPending = true;
        dataService.setUnreachableState();
        APIUtils.hostPowerOn()
            .then(() => {
              // Check for on state
              return checkHostStatus(
                  Constants.HOST_STATE_TEXT.on, Constants.TIMEOUT.HOST_ON,
                  Constants.MESSAGES.POLL.HOST_ON_TIMEOUT);
            })
            .catch(error => {
              console.log(error);
              toastService.error(Constants.MESSAGES.POWER_OP.POWER_ON_FAILED);
            })
            .finally(() => {
              $scope.operationPending = false;
            });
      };

      /*
       *  Power operations modal
       */
      const initPowerOperation = function(powerOperation) {
        switch (powerOperation) {
          case powerOperations.WARM_REBOOT:
            warmReboot();
            break;
          case powerOperations.COLD_REBOOT:
            coldReboot();
            break;
          case powerOperations.WARM_SHUTDOWN:
            orderlyShutdown();
            break;
          case powerOperations.COLD_SHUTDOWN:
            immediateShutdown();
            break;
          default:
            // do nothing
        }
      };

      const powerOperationModal = function() {
        $uibModal
            .open({
              template: modalTemplate,
              windowTopClass: 'uib-modal',
              scope: $scope,
              ariaLabelledBy: 'modal-operation'
            })
            .result
            .then(function(activeModal) {
              initPowerOperation(activeModal);
            })
            .finally(function() {
              $scope.activeModal = undefined;
            });
      };

      $scope.rebootConfirmModal = function() {
        if ($scope.rebootForm.radioReboot.$modelValue == 'warm-reboot') {
          $scope.activeModal = powerOperations.WARM_REBOOT;
        } else if ($scope.rebootForm.radioReboot.$modelValue == 'cold-reboot') {
          $scope.activeModal = powerOperations.COLD_REBOOT;
        }
        powerOperationModal();
      };

      $scope.shutdownConfirmModal = function() {
        if ($scope.shutdownForm.radioShutdown.$modelValue == 'warm-shutdown') {
          $scope.activeModal = powerOperations.WARM_SHUTDOWN;
        } else if (
            $scope.shutdownForm.radioShutdown.$modelValue == 'cold-shutdown') {
          $scope.activeModal = powerOperations.COLD_SHUTDOWN;
        }
        powerOperationModal();
      };

      $scope.resetForm = function() {
        $scope.boot = angular.copy($scope.originalBoot);
        $scope.TPMToggle = angular.copy($scope.originalTPMToggle);
      };

      /*
       *   Get boot settings
       */
      const loadBootSettings = function() {
        APIUtils.getBootOptions()
            .then(function(response) {
              const boot = response.Boot;
              const BootSourceOverrideEnabled =
                  boot['BootSourceOverrideEnabled'];
              const BootSourceOverrideTarget = boot['BootSourceOverrideTarget'];
              const bootSourceValues =
                  boot['BootSourceOverrideTarget@Redfish.AllowableValues'];

              $scope.bootSources = bootSourceValues;

              $scope.boot = {
                BootSourceOverrideEnabled: BootSourceOverrideEnabled,
                BootSourceOverrideTarget: BootSourceOverrideTarget
              };

              if (BootSourceOverrideEnabled == 'Once') {
                $scope.boot.oneTimeBootEnabled = true;
              }

              $scope.originalBoot = angular.copy($scope.boot);
            })
            .catch(function(error) {
              $scope.bootOverrideError = true;
              toastService.error('Unable to get boot override values.');
              console.log(
                  'Error loading boot settings:', JSON.stringify(error));
            });
        $scope.loading = false;
      };

      /*
       *   Get TPM status
       */
      const loadTPMStatus = function() {
        APIUtils.getTPMStatus()
            .then(function(response) {
              $scope.TPMToggle = response.data;
              $scope.originalTPMToggle = angular.copy($scope.TPMToggle);
            })
            .catch(function(error) {
              toastService.error('Unable to get TPM policy status.');
              console.log('Error loading TPM status', JSON.stringify(error));
            });
        $scope.loading = false;
      };

      /*
       *   Save boot settings
       */
      $scope.saveBootSettings = function() {
        if ($scope.hostBootSettings.bootSelected.$dirty ||
            $scope.hostBootSettings.oneTime.$dirty) {
          const data = {};
          data.Boot = {};

          let isOneTimeBoot = $scope.boot.oneTimeBootEnabled;
          let overrideTarget = $scope.boot.BootSourceOverrideTarget || 'None';
          let overrideEnabled = 'Disabled';

          if (isOneTimeBoot) {
            overrideEnabled = 'Once';
          } else if (overrideTarget !== 'None') {
            overrideEnabled = 'Continuous';
          }

          data.Boot.BootSourceOverrideEnabled = overrideEnabled;
          data.Boot.BootSourceOverrideTarget = overrideTarget;

          APIUtils.saveBootSettings(data).then(
              function(response) {
                $scope.originalBoot = angular.copy($scope.boot);
                toastService.success('Successfully updated boot settings.');
              },
              function(error) {
                toastService.error('Unable to save boot settings.');
                console.log(JSON.stringify(error));
              });
        }
      };

      /*
       *   Save TPM required policy
       */
      $scope.saveTPMPolicy = function() {
        if ($scope.hostBootSettings.toggle.$dirty) {
          const tpmEnabled = $scope.TPMToggle.TPMEnable;

          if (tpmEnabled === undefined) {
            return;
          }

          APIUtils.saveTPMEnable(tpmEnabled)
              .then(
                  function(response) {
                    $scope.originalTPMToggle = angular.copy($scope.TPMToggle);
                    toastService.success(
                        'Sucessfully updated TPM required policy.');
                  },
                  function(error) {
                    toastService.error('Unable to update TPM required policy.');
                    console.log(JSON.stringify(error));
                  });
        }
      };

      /**
       * Callback when boot setting option changed
       */
      $scope.onChangeBootSetting = function() {
        const bootSetting = $scope.hostBootSettings.bootSelected.$viewValue;
        if (bootSetting === 'None') {
          $scope.boot.oneTimeBootEnabled = false;
        }
      };

      /*
       *   Emitted every time the view is reloaded
       */
      $scope.$on('$viewContentLoaded', function() {
        APIUtils.getLastPowerTime()
            .then(
                function(data) {
                  if (data.data == 0) {
                    $scope.powerTime = 'not available';
                  } else {
                    $scope.powerTime = data.data;
                  }
                },
                function(error) {
                  toastService.error(
                      'Unable to get last power operation time.');
                  console.log(JSON.stringify(error));
                })
            .finally(function() {
              $scope.loading = false;
            });

        loadBootSettings();
        loadTPMStatus();
      });
    }
  ]);
})(angular);
