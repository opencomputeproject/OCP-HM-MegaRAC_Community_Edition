/**
 * Controller for Restore Factory Default
 *
 * @module app/serverControl
 * @exports restoreDefaultController
 * @name restoreDefaultController
 */

window.angular && (function(angular) {
    'use strict';
  
    angular.module('app.serverControl').controller('restoreDefaultController', [
      '$scope', '$window', 'APIUtils', 'dataService', '$location',
      '$anchorScroll', 'Constants', '$interval', '$q', '$timeout', 'toastService',
      function(
          $scope, $window, APIUtils, dataService, $location, $anchorScroll,
          Constants, $interval, $q, $timeout, toastService) {
          $scope.dataService = dataService;
          $scope.confirmRestoreDefault = false;
          $scope.loading = false;
          
          $scope.RestoreDefault = function(){
            $scope.confirmRestoreDefault = true;
          };
          $scope.ProceedRestoreDefault = function(){
            $scope.confirmRestoreDefault = false;
            $scope.loading = true;
            var promises = [];
            promises.push(resetDefaultNetwork());
            promises.push(resetDefaultBMC());

            if (promises.length) {
              $q.all(promises).then(
                function (response){
                  $timeout(function() {
                    rebootBMC();
                  }, 3000);
                },
                function(error) {
                  $scope.loading = false;
                  toastService.error('Something Went Wrong!!');
                }
              );
            }
          };
          function resetDefaultNetwork(){
            return APIUtils.resetDefaultNetwork().then(
              function(response){},
              function (error){
                console.log(JSON.stringify(error));
                  return $q.reject();
              }
            );
          }

          function resetDefaultBMC(){
            return APIUtils.resetDefaultBMC().then(
              function(response){},
              function(error){
                console.log(JSON.stringify(error));
                  return $q.reject();
              }
            );
          };
          function rebootBMC() {
            APIUtils.bmcReboot().then(
              function(response) {
                toastService.success('BMC is rebooting.');
                $scope.loading = false;
              },
              function(error) {
                console.log(JSON.stringify(error));
                toastService.error('Unable to reboot BMC.');
                $scope.loading = false;
              }
            );
          }
      }
    ]);
  })(angular);
  