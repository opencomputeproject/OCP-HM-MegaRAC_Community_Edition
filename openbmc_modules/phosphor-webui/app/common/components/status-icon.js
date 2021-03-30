window.angular && (function(angular) {
  'use strict';

  /**
   * statusIcon Component
   *
   * To use:
   * The <status-icon> component expects a 'status' attribute
   * with a status value (on, off, warn, error)
   *
   */

  /**
   * statusIcon Component template
   */
  const template = `<icon ng-if="$ctrl.status === 'on'"
                          file="icon-on.svg"
                          aria-hidden="true"
                          class="status-icon">
                    </icon>
                    <icon ng-if="$ctrl.status === 'off'"
                          file="icon-off.svg"
                          aria-hidden="true"
                          class="status-icon">
                    </icon>
                    <icon ng-if="$ctrl.status === 'warn'"
                          file="icon-warning.svg"
                          aria-hidden="true"
                          class="status-icon">
                    </icon>
                    <icon ng-if="$ctrl.status === 'error'"
                          file="icon-critical.svg"
                          aria-hidden="true"
                          class="status-icon">
                    </icon>
                    <icon ng-if="$ctrl.status === 'loading'"
                          file="icon-loader.svg"
                          aria-hidden="true"
                          class="icon__loader icon__loader-sm">
                    </icon>
                    `

  /**
   * Register statusIcon component
   */
  angular.module('app.common.components')
      .component('statusIcon', {template, bindings: {status: '@'}})
})(window.angular);