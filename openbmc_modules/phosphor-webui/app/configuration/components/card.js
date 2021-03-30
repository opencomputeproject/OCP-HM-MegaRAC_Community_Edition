window.angular && (function(angular) {
  'use strict';

  /**
   *
   * firmwareCard Component
   *
   */

  /**
   * Component template
   */
  const template = `
    <div class="card-component">
      <div class="card__header"
           ng-class="{
            'card__header--success' : $ctrl.status === 'success',
            'card__header--error'   : $ctrl.status === 'error' }">
        <p class="card__header__label inline">{{$ctrl.headerLabel}}</p>
        <p class="card__header__value inline">{{$ctrl.headerValue}}</p>
      </div>
      <div class="card__body"
           ng-if="$ctrl.body">
        <div class="row">
          <div class="column small-6">
            <label>BMC Status</label>
            {{$ctrl.bmcStatus || 'n/a'}}
          </div>
          <div class="column small-6">
            <label>Host status</label>
            {{$ctrl.hostStatus || 'n/a'}}
          </div>
        </div>
      </div>
    </div>`

  /**
   * Register firmwareCard component
   */
  angular.module('app.configuration').component('firmwareCard', {
    template,
    bindings: {
      headerLabel: '@',
      headerValue: '<',
      status: '<',  // optional, 'success' or 'error'
      body: '<',    // boolean true to render body content
      hostStatus: '<',
      bmcStatus: '<'
    }
  })
})(window.angular);