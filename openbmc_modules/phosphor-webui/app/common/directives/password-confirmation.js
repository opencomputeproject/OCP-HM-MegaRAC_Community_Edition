window.angular && (function(angular) {
  'use strict';

  /**
   * Password confirmation validator
   *
   * To use, add attribute directive to password confirmation input field
   * Also include attribute 'first-password' with value set to first password
   * to check against
   *
   * <input password-confirmation first-password="ctrl.password"
   * name="passwordConfirm">
   *
   */
  angular.module('app.common.directives')
      .directive('passwordConfirm', function() {
        return {
          restrict: 'A',
          require: 'ngModel',
          scope: {firstPassword: '='},
          link: function(scope, element, attrs, controller) {
            if (controller === undefined) {
              return;
            }
            controller.$validators.passwordConfirm =
                (modelValue, viewValue) => {
                  const firstPassword =
                      scope.firstPassword ? scope.firstPassword : '';
                  const secondPassword = modelValue || viewValue || '';
                  if (firstPassword == secondPassword) {
                    return true;
                  } else {
                    return false;
                  }
                };
            element.on('keyup', () => {
              controller.$validate();
            });
          }
        };
      });
})(window.angular);
