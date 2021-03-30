window.angular && (function(angular) {
  'use strict';

  /**
   * alertBanner Component
   */


  /**
   * alertBanner Component controller
   */
  const controller = function() {
    this.status;
    this.$onInit = () => {
      switch (this.type) {
        case 'warn':
        case 'error':
          this.status = this.type;
          break;
        case 'success':
          this.status = 'on';
          break;
        default:
      }
    };
  };

  /**
   * alertBanner Component template
   */
  const template = `
    <div  class="alert-banner"
          ng-class="{
            'alert-banner--info': $ctrl.type === 'info',
            'alert-banner--warn': $ctrl.type === 'warn',
            'alert-banner--error': $ctrl.type === 'error',
            'alert-banner--success': $ctrl.type === 'success'}">
      <status-icon
        ng-if="$ctrl.type !== 'info'"
        status="{{$ctrl.status}}"
        class="status-icon">
      </status-icon>
      <ng-bind-html
        ng-bind-html="$ctrl.bannerText || ''">
      </ng-bind-html>
    </div>
  `

  /**
   * Register alertBanner component
   */
  angular.module('app.common.components').component('alertBanner', {
    template,
    controller,
    bindings: {
      type: '@',       // string 'info', 'warn', 'error' or 'success'
      bannerText: '<'  // string, can include valid HTML
    }
  })
})(window.angular);