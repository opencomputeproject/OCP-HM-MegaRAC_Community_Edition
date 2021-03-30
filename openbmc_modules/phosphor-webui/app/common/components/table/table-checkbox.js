window.angular && (function(angular) {
  'use strict';

  /**
   *
   * tableCheckbox Component
   *
   */

  const controller = function($element) {
    // <input> element ref needed to add indeterminate state
    let inputEl;

    /**
     * Callback when the input select value changes
     */
    this.onSelectChange = () => {
      const checked = this.ngModel;
      this.emitChange({checked});
    };

    /**
     * onChanges Component lifecycle hook
     */
    this.$onChanges = (onChangesObj) => {
      const indeterminateChange = onChangesObj.indeterminate;
      if (indeterminateChange && inputEl) {
        inputEl.prop('indeterminate', this.indeterminate);
      }
    };

    /**
     * postLink Component lifecycle hook
     */
    this.$postLink = () => {
      inputEl = $element.find('input');
    };
  };

  /**
   * Component template
   */
  const template = `
    <div class="bmc-table__checkbox-container">
      <label class="bmc-table__checkbox"
             ng-class="{
              'checked': $ctrl.ngModel,
              'indeterminate': $ctrl.indeterminate
            }">
        <input type="checkbox"
            class="bmc-table__checkbox-input"
            ng-model="$ctrl.ngModel"
            ng-change="$ctrl.onSelectChange()"
            aria-label="Select row"/>
        <span class="screen-reader-offscreen">Select row</span>
      </label>
    </div>`

  /**
   * Register tableCheckbox component
   */
  angular.module('app.common.components').component('tableCheckbox', {
    controller: ['$element', controller],
    template,
    bindings: {ngModel: '=', indeterminate: '<', emitChange: '&'}
  })
})(window.angular);