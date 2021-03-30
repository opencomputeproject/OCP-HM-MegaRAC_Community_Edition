window.angular && (function(angular) {
  'use strict';

  /**
   * Username validator
   *
   * Checks if entered username is a duplicate
   * Provide existingUsernames scope that should be an array of
   * existing usernames
   *
   * <input username-validator  existing-usernames="[]"/>
   *
   */
  angular.module('app.accessControl')
      .directive('usernameValidator', function() {
        return {
          restrict: 'A', require: 'ngModel', scope: {existingUsernames: '='},
              link: function(scope, element, attrs, controller) {
                if (scope.existingUsernames === undefined) {
                  return;
                }
                controller.$validators.duplicateUsername =
                    (modelValue, viewValue) => {
                      const enteredUsername = modelValue || viewValue;
                      const matchedExisting = scope.existingUsernames.find(
                          (username) => username === enteredUsername);
                      if (matchedExisting) {
                        return false;
                      } else {
                        return true;
                      }
                    };
                element.on('blur', () => {
                  controller.$validate();
                });
              }
        }
      });
})(window.angular);
