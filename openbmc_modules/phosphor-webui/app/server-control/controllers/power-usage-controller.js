/**
 * Controller for power-usage
 *
 * @module app/serverControl
 * @exports powerUsageController
 * @name powerUsageController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.serverControl').controller('powerUsageController', [
    '$scope', '$window', 'APIUtils', '$route', '$q', 'toastService',
    function($scope, $window, APIUtils, $route, $q, toastService) {
      $scope.power_consumption = '';
      $scope.power_cap = {};
      $scope.loading = false;
      loadPowerData();

      function loadPowerData() {
        $scope.loading = true;

        var getPowerCapPromise = APIUtils.getPowerCap().then(
            function(data) {
              $scope.power_cap = data.data;
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

        var promises = [
          getPowerConsumptionPromise,
          getPowerCapPromise,
        ];

        $q.all(promises).finally(function() {
          $scope.loading = false;
        });
      }

      $scope.setPowerCap = function() {
        // The power cap value will be undefined if outside range
        if (!$scope.power_cap.PowerCap) {
          toastService.error(
              'Power cap value between 100 and 10,000 is required');
          return;
        }
        $scope.loading = true;
        var promises = [
          setPowerCapValue(),
          setPowerCapEnable(),
        ];

        $q.all(promises)
            .then(
                function() {
                  toastService.success('Power cap settings saved');
                },
                function(errors) {
                  toastService.error('Power cap settings could not be saved');
                })
            .finally(function() {
              $scope.loading = false;
            });
      };
      $scope.refresh = function() {
        $route.reload();
      };

      function setPowerCapValue() {
        return APIUtils.setPowerCap($scope.power_cap.PowerCap)
            .then(
                function(data) {},
                function(error) {
                  console.log(JSON.stringify(error));
                  return $q.reject();
                });
      }

      function setPowerCapEnable() {
        return APIUtils.setPowerCapEnable($scope.power_cap.PowerCapEnable)
            .then(
                function(data) {},
                function(error) {
                  console.log(JSON.stringify(error));
                  return $q.reject();
                });
      }
    }
  ]);
})(angular);
