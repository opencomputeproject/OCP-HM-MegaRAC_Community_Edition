/**
 * Controller for systemOverview
 *
 * @module app/overview
 * @exports systemOverviewController
 * @name systemOverviewController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.overview').controller('systemOverviewController', [
    '$scope', '$window', 'APIUtils', 'dataService', 'Constants', '$q',
    function($scope, $window, APIUtils, dataService, Constants, $q) {
      $scope.dataService = dataService;
      $scope.dropdown_selected = false;
      $scope.logs = [];
      $scope.server_info = {};
      $scope.bmc_firmware = '';
      $scope.bmc_time = '';
      $scope.server_firmware = '';
      $scope.power_consumption = '';
      $scope.power_cap = '';
      $scope.bmc_ip_addresses = [];
      $scope.loading = false;
      $scope.edit_hostname = false;
      $scope.newHostname = '';

      loadOverviewData();

      function loadOverviewData() {
        $scope.loading = true;

        var getLogsPromise = APIUtils.getLogs().then(
            function(data) {
              $scope.logs = data.data.filter(function(log) {
                return log.severity_flags.high == true;
              });
            },
            function(error) {
              console.log(JSON.stringify(error));
            });

        var getFirmwaresPromise = APIUtils.getFirmwares().then(
            function(data) {
              $scope.bmc_firmware = data.bmcActiveVersion;
              $scope.server_firmware = data.hostActiveVersion;
            },
            function(error) {
              console.log(JSON.stringify(error));
            });

        var getLEDStatePromise = APIUtils.getLEDState().then(
            function(data) {
              if (data == APIUtils.LED_STATE.on) {
                dataService.LED_state = APIUtils.LED_STATE_TEXT.on;
              } else {
                dataService.LED_state = APIUtils.LED_STATE_TEXT.off;
              }
            },
            function(error) {
              console.log(JSON.stringify(error));
            });

        var getBMCTimePromise = APIUtils.getBMCTime().then(
            function(data) {
              $scope.bmc_time = data.data.Elapsed / 1000;
            },
            function(error) {
              console.log(JSON.stringify(error));
            });

        var getServerInfoPromise = APIUtils.getServerInfo().then(
            function(data) {
              $scope.server_info = data.data;
            },
            function(error) {
              console.log(JSON.stringify(error));
            });

        var getPowerConsumptionPromise = APIUtils.getPowerConsumption().then(
            function(data) {
              $scope.power_consumption = data;
            },
            function(error) {
              console.log(JSON.stringify(error));
            });

        var getPowerCapPromise = APIUtils.getPowerCap().then(
            function(data) {
              if (data.data.PowerCapEnable == false) {
                $scope.power_cap = Constants.POWER_CAP_TEXT.disabled;
              } else {
                $scope.power_cap =
                    data.data.PowerCap + ' ' + Constants.POWER_CAP_TEXT.unit;
              }
            },
            function(error) {
              console.log(JSON.stringify(error));
            });

        var getNetworkInfoPromise = APIUtils.getNetworkInfo().then(
            function(data) {
              // TODO: openbmc/openbmc#3150 Support IPV6 when
              // officially supported by the backend
              $scope.bmc_ip_addresses = data.formatted_data.ip_addresses.ipv4;
              $scope.newHostname = data.hostname;
            },
            function(error) {
              console.log(JSON.stringify(error));
            });

        var promises = [
          getLogsPromise,
          getFirmwaresPromise,
          getLEDStatePromise,
          getBMCTimePromise,
          getServerInfoPromise,
          getPowerConsumptionPromise,
          getPowerCapPromise,
          getNetworkInfoPromise,
        ];

        $q.all(promises).finally(function() {
          $scope.loading = false;
        });
      }

      $scope.toggleLED = function() {
        var toggleState =
            (dataService.LED_state == APIUtils.LED_STATE_TEXT.on) ?
            APIUtils.LED_STATE.off :
            APIUtils.LED_STATE.on;
        dataService.LED_state =
            (dataService.LED_state == APIUtils.LED_STATE_TEXT.on) ?
            APIUtils.LED_STATE_TEXT.off :
            APIUtils.LED_STATE_TEXT.on;
        APIUtils.setLEDState(toggleState, function(status) {});
      };

      $scope.saveHostname = function(hostname) {
        $scope.edit_hostname = false;
        $scope.loading = true;
        APIUtils.setHostname(hostname).then(
            function(data) {
              APIUtils.getNetworkInfo().then(function(data) {
                dataService.setNetworkInfo(data);
              });
            },
            function(error) {
              console.log(error);
            });
        $scope.loading = false;
      };

      $scope.getEventLogTitle = function(event) {
        var title = event.type;
        if ((event.eventID != 'None') && (event.description != 'None')) {
          title = event.eventID + ': ' + event.description;
        }
        return title;
      };
    }
  ]);
})(angular);
