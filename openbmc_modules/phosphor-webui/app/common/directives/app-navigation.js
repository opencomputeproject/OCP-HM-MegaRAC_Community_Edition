window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives')
      .directive('appNavigation', function() {
        return {
          'restrict': 'E',
          'template': require('./app-navigation.html'),
          'scope': {'path': '=', 'showNavigation': '='},
          'controller': [
            '$scope', '$location', 'dataService',
            function($scope, $location, dataService) {
              $scope.showHealthMenu = false;
              $scope.showControlMenu = false;
              $scope.showConfigMenu = false;
              $scope.showAccessMenu = false;
              $scope.dataService = dataService;

              $scope.change = function(firstLevel) {
                switch (firstLevel) {
                  case 'server-health':
                    $scope.showHealthMenu = !$scope.showHealthMenu;
                    break;
                  case 'server-control':
                    $scope.showControlMenu = !$scope.showControlMenu;
                    break;
                  case 'configuration':
                    $scope.showConfigMenu = !$scope.showConfigMenu;
                    break;
                  case 'access-control':
                    $scope.showAccessMenu = !$scope.showAccessMenu;
                    break;
                  case 'overview':
                    $location.url('/overview/server');
                    break;
                };
              };
              $scope.$watch('path', function() {
                var urlRoot = $location.path().split('/')[1];
                if (urlRoot != '') {
                  $scope.firstLevel = urlRoot;
                } else {
                  $scope.firstLevel = 'overview';
                }
                $scope.showSubMenu = true;
              });
              $scope.$watch('showNavigation', function() {
                var urlRoot = $location.path().split('/')[1];
                if (urlRoot != '') {
                  $scope.firstLevel = urlRoot;
                } else {
                  $scope.firstLevel = 'overview';
                }
              });
            }
          ],
          link: function(scope, element, attributes) {
            var rawNavElement = angular.element(element)[0];
            angular.element(window.document).bind('click', function(event) {
              if (rawNavElement.contains(event.target)) return;

              if (scope.showSubMenu) {
                scope.$apply(function() {
                  scope.showSubMenu = true;
                });
              }
            });
          }
        };
      });
})(window.angular);