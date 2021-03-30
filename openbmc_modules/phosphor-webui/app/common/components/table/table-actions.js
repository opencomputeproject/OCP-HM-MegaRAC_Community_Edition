window.angular && (function(angular) {
  'use strict';

  /**
   *
   * tableActions Component
   *
   * To use:
   * The <table-actions> component expects an 'actions' attribute
   * that should be an array of action objects.
   * Each action object should have 'type', 'enabled', and 'file'
   * properties.
   *
   * actions: [
   *  {type: 'Edit', enabled: true, file: 'icon-edit.svg'},
   *  {type: 'Delete', enabled: false, file: 'icon-trashcan.svg'}
   * ]
   *
   * The 'type' property is a string value that will be emitted to the
   * parent component when clicked.
   *
   * The 'enabled' property is a boolean that will enable/disable
   * the button.
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
            if (action.enabled === undefined) {
              action.enabled = true;
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

    /**
     * onChanges Component lifecycle hook
     */
    this.$onChanges = () => {
      this.actions = setActions(this.actions);
    };
  };

  /**
   * Component template
   */
  const template = `
    <button
      class="btn  btn-tertiary"
      type="button"
      aria-label="{{action.type}}"
      ng-repeat="action in $ctrl.actions track by $index"
      ng-disabled="!action.enabled"
      ng-click="$ctrl.onClick(action.type)">
      <icon ng-if="action.file" ng-file="{{action.file}}"></icon>
      <span ng-if="!action.file">{{action.type}}</span>
    </button>`

  /**
   * Register tableActions component
   */
  angular.module('app.common.components').component('tableActions', {
    controller,
    template,
    bindings: {actions: '<', emitAction: '&'}
  })
})(window.angular);