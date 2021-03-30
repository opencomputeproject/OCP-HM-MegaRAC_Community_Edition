angular.module('app.common.directives').directive('hasError', function() {
  return {
    scope: {hasError: '='},
    require: 'ngModel',
    link: function(scope, elm, attrs, ngModel) {
      scope.$watch('hasError', function(value) {
        ngModel.$setValidity('hasError', value ? false : true);
      });
    }
  };
});
