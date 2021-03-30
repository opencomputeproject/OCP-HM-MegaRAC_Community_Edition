/**
 * Controller for server
 *
 * @module app/serverControl
 * @exports remoteConsoleWindowController
 * @name remoteConsoleController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.serverControl')
      .controller('remoteConsoleWindowController', [
        '$scope', '$window', 'dataService',
        function($scope, $window, dataService) {
          $scope.dataService = dataService;
          dataService.showNavigation = false;
          dataService.bodyStyle = {'background': 'white'};

          $scope.close = function() {
            $window.close();
          };
        }
      ]);
})(angular);
