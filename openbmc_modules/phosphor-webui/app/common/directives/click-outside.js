window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('clickOutside', [
    '$document',
    function($document) {
      return {
        restrict: 'A', scope: {clickOutside: '&'},
            link: function(scope, el, attr) {
              function clickEvent(e) {
                if (el !== e.target && !el[0].contains(e.target)) {
                  scope.$apply(function() {
                    scope.$eval(scope.clickOutside);
                  });
                }
              }
              $document.bind('click', clickEvent);

              scope.$on('$destroy', function() {
                $document.unbind('click', clickEvent);
              });
            }
      }
    }
  ]);
})(window.angular);