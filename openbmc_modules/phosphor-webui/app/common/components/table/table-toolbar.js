window.angular && (function(angular) {
  'use strict';

  /**
   *
   * tableToolbar Component
   *
   * To use:
   * The <table-toolbar> component expects an 'actions' attribute
   * that should be an array of action objects.
   * Each action object should have 'type', 'label', and 'file'
   * properties.
   *
   * actions: [
   *  {type: 'Edit', label: 'Edit', file: 'icon-edit.svg'},
   *  {type: 'Delete', label: 'Edit', file: 'icon-trashcan.svg'}
   * ]
   *
   * The 'type' property is a string value that will be emitted to the
   * parent component when clicked.
   *
   * The 'label' property is a string value that will render as text in
   * the button
   *
   * The 'file' property is a string value of the filename of the svg icon
   * to provide <icon> directive.
   *
   */

  const controller = function() {
    /**
     * Set defaults if properties undefined
     * @param {[]} actions
     */
    const setActions = (actions = []) => {
      return actions
          .map((action) => {
            if (action.type === undefined) {
              return;
            }
            if (action.file === undefined) {
              action.file = null;
            }
            return action;
          })
          .filter((action) => action);
    };

    /**
     * Callback when button action clicked
     * Emits the action type to the parent component
     */
    this.onClick = (action) => {
      this.emitAction({action});
    };

    this.onClickClose = () => {
      this.emitClose();
    };

    /**
     * onInit Component lifecycle hook
     */
    this.$onInit = () => {
      this.actions = setActions(this.actions);
    };
  };

  /**
   * Component template
   */
  const template = `
    <div class="bmc-table__toolbar" ng-if="$ctrl.active">
      <p class="toolbar__selection-count">{{$ctrl.selectionCount}} selected</p>
      <div class="toolbar__batch-actions" ng-show="$ctrl.actions.length > 0">
        <button
          class="btn  btn-tertiary"
          type="button"
          aria-label="{{action.label}}"
          ng-repeat="action in $ctrl.actions"
          ng-click="$ctrl.onClick(action.type)">
          <icon ng-if="action.file !== null"
                ng-file="{{action.file}}"
                aria-hidden="true">
          </icon>
          {{action.label || action.type}}
        </button>
        <button
          class="btn  btn-tertiary  btn-close"
          type="button"
          aria-label="Cancel"
          ng-click="$ctrl.onClickClose()">
          Cancel
        </button>
      </div>
    </div>`

  /**
   * Register tableToolbar component
   */
  angular.module('app.common.components').component('tableToolbar', {
    controller,
    template,
    bindings: {
      actions: '<',
      selectionCount: '<',
      active: '<',
      emitAction: '&',
      emitClose: '&'
    }
  })
})(window.angular);