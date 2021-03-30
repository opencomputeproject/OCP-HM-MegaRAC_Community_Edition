window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('confirm', [
    '$timeout',
    function($timeout) {
      return {
        'restrict': 'E',
        'template': require('./confirm.html'),
        'scope':
            {'title': '@', 'message': '@', 'confirm': '=', 'callback': '='},
        'controller': [
          '$scope',
          function($scope) {
            $scope.cancel = function() {
              $scope.confirm = false;
              $scope.$parent.confirm = false;
            };
            $scope.accept = function() {
              $scope.callback();
              $scope.cancel();
            };
          }
        ],
        link: function(scope, e) {
          scope.$watch('confirm', function() {
            if (scope.confirm) {
              $timeout(function() {
                angular.element(e[0].parentNode).css({
                  'min-height':
                      e[0].querySelector('.inline__confirm').offsetHeight + 'px'
                });
              }, 0);
            } else {
              angular.element(e[0].parentNode).css({'min-height': 0 + 'px'});
            }
          });
        }
      };
    }
  ]);
})(window.angular);
