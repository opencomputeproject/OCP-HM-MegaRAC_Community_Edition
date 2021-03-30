/**
 * Controller for SNMP
 *
 * @module app/configuration
 * @exports snmpController
 * @name snmpController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.configuration').controller('snmpController', [
    '$scope', '$window', 'APIUtils', '$route', '$q', 'toastService',
    function($scope, $window, APIUtils, $route, $q, toastService) {
      $scope.managers = [];
      $scope.loading = true;
      $scope.managersToDelete = [];

      var getSNMPManagers = APIUtils.getSNMPManagers().then(
          function(data) {
            // Convert to array of objects from an object of objects, easier
            // to manipulate (e.g. add/remove). Convert key to a path property.
            for (var key in data.data) {
              $scope.managers.push({
                path: key,
                port: data.data[key].Port,
                updatePort: false,
                address: data.data[key].Address,
                updateAddress: false
              })
            }
          },
          function(error) {
            toastService.error('Unable to load SNMP settings.');
            console.log(JSON.stringify(error));
          });

      getSNMPManagers.finally(function() {
        $scope.loading = false;
      });

      $scope.addNewSNMPManager = function() {
        $scope.managers.push({address: '', port: ''});
      };

      $scope.removeSNMPManager = function(index) {
        // If the SNMP Manager has a path it exists on the backend and we
        // need to make a call to remove it
        if ($scope.managers[index].path) {
          $scope.managersToDelete.push($scope.managers[index].path);
        }
        $scope.managers.splice(index, 1);
      };

      $scope.refresh = function() {
        $route.reload();
      };

      $scope.setSNMP = function() {
        $scope.loading = true;
        var promises = [];

        // Validate that no field are empty and port is valid. Port value is
        // undefined if it is an invalid number.
        for (let i in $scope.managers) {
          if (!$scope.managers[i].address || !$scope.managers[i].port) {
            $scope.loading = false;
            toastService.error('Cannot save. Please resolve errors on page.');
            return;
          }
        }
        // Iterate in reverse so can splice
        // https://stackoverflow.com/questions/9882284/looping-through-array-and-removing-items-without-breaking-for-loop
        let managersLength = $scope.managers.length;
        while (managersLength--) {
          // If the manager does not have a 'path', it is a new manager
          // and needs to be created
          if (!$scope.managers[managersLength].path) {
            promises.push(addManager(
                $scope.managers[managersLength].address,
                $scope.managers[managersLength].port));
          } else {
            if ($scope.managers[managersLength].updateAddress) {
              promises.push(setManagerAddress(
                  $scope.managers[managersLength].path,
                  $scope.managers[managersLength].address));
            }
            if ($scope.managers[managersLength].updatePort) {
              promises.push(setManagerPort(
                  $scope.managers[managersLength].path,
                  $scope.managers[managersLength].port));
            }
          }
        }

        // Add delete promises last since we might be adding to
        // managersToDelete above
        for (let i in $scope.managersToDelete) {
          promises.push(deleteManager($scope.managersToDelete[i]));
        }

        $q.all(promises)
            .then(
                function() {
                  $scope.refresh();
                  toastService.success('SNMP settings have been saved.');
                },
                function(errors) {
                  toastService.error('Unable to set SNMP Managers.');
                  console.log(JSON.stringify(errors));
                })
            .finally(function() {
              $scope.loading = false;
            });
      };

      function addManager(address, port) {
        return APIUtils.addSNMPManager(address, port);
      }

      function deleteManager(path) {
        return APIUtils.deleteObject(path);
      }

      function setManagerAddress(path, address) {
        return APIUtils.setSNMPManagerAddress(path, address);
      }

      function setManagerPort(path, port) {
        return APIUtils.setSNMPManagerPort(path, port);
      }
    }
  ]);
})(angular);
