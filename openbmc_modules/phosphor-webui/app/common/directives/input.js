window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives')
      .directive('setFocusOnNewInput', function() {
        return function(scope, element, attrs) {
          var elem = window.document.getElementById(element[0].id);
          // Focus on the newly created input.
          // Since this directive is also called when initializing fields
          // on a page load, need to determine if the call is from a page load
          // or from the user pressing the "Add new" button. The easiest way to
          // do this is to check if the field is empty, if it is we know this is
          // a new field since all empty fields are removed from the array.
          if (!scope[attrs.ngModel] && elem) {
            elem.focus();
          }
        };
      });
})(window.angular);
