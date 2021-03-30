window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('toggleFlag', [
    '$document',
    function($document) {
      return {
        restrict: 'A',
        link: function(scope, element, attrs) {
          function elementClick(e) {
            e.stopPropagation();
          }

          function documentClick(e) {
            scope[attrs.toggleFlag] = false;
            scope.$apply();
          }

          element.on('click', elementClick);
          $document.on('click', documentClick);

          // remove event handlers when directive is destroyed
          scope.$on('$destroy', function() {
            element.off('click', elementClick);
            $document.off('click', documentClick);
          });
        }
      };
    }
  ]);
})(window.angular);
