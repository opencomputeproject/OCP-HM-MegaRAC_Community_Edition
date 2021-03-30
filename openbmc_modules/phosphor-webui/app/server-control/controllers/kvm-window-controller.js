/**
 * Controller for kvm window
 *
 * @module app/serverControl
 * @exports kvmWindowController
 * @name kvmWindowController
 */

window.angular && (function(angular) {
  'use strict';
  angular.module('app.serverControl').controller('kvmWindowController', [
    '$scope', '$window', 'dataService',
    function($scope, $window, dataService) {
      $scope.dataService = dataService;
      dataService.showNavigation = false;
      dataService.bodyStyle = {background: 'white'};

      $scope.close = function() {
        $window.close();
        if (rfb) {
          rfb.disconnect();
        }
      };
    }
  ]);
})(angular);