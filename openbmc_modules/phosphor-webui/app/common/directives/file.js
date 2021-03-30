window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('file', function() {
    return {
      scope: {file: '='},
      link: function(scope, el, attrs) {
        el.bind('change', function(event) {
          var file = event.target.files[0];
          scope.file = file ? file : undefined;
          scope.$apply();
        });
      }
    };
  });
})(window.angular);
