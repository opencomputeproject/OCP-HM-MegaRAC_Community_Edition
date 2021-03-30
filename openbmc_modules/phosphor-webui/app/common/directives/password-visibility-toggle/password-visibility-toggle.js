window.angular && (function(angular) {
  'use strict';

  /**
   * Password visibility toggle
   *
   * This attribute directive will toggle an input type
   * from password/text to show/hide the value of a password
   * input field.
   *
   * To use:
   * <input type="password" password-visibility-toggle />
   */
  angular.module('app.common.directives')
      .directive('passwordVisibilityToggle', [
        '$compile',
        function($compile) {
          return {
            restrict: 'A',
            link: function(scope, element) {
              const instanceScope = scope.$new();
              const buttonTemplate = `
              <button type="button"
                      aria-hidden="true"
                      class="btn  btn-tertiary btn-password-toggle"
                      ng-class="{
                        'password-visible': visible,
                        'password-hidden': !visible
                      }"
                ng-click="toggleField()">
                <icon ng-if="!visible" file="icon-visibility-on.svg"></icon>
                <icon ng-if="visible" file="icon-visibility-off.svg"></icon>
              </button>`;

              instanceScope.visible = false;
              instanceScope.toggleField = () => {
                instanceScope.visible = !instanceScope.visible;
                const type = instanceScope.visible ? 'text' : 'password';
                element.attr('type', type);
              };
              element.after($compile(buttonTemplate)(instanceScope));
            }
          };
        }
      ]);
})(window.angular);
