window.angular && (function(angular) {
  'use strict';

  /**
   *
   * bmcTable Component
   *
   * To use:
   *
   * The 'data' attribute should be an array of all row objects in the table.
   * It will render each item as a <tr> in the table.
   * Each row object in the data array should also have a 'uiData'
   * property that should be an array of the properties that will render
   * as each table cell <td>.
   * Each row object in the data array can optionally have an
   * 'actions' property that should be an array of actions to provide the
   * <bmc-table-actions> component.
   * Each row object can optionally have an 'expandContent' property
   * that should be a string value and can contain valid HTML. To render
   * the expanded content, set 'expandable' attribute to true.
   * Each row object can optionally have a 'selectable' property. Defaults
   * to true if table is selectable. If a particular row should not
   * be selectable, set to false.
   *
   * data = [
   *  { uiData: ['root', 'Admin', 'enabled' ], selectable: false },
   *  { uiData: ['user1', 'User', 'disabled' ] }
   * ]
   *
   * The 'header' attribute should be an array of all header objects in the
   * table. Each object in the header array should have a 'label' property
   * that will render as a <th> in the table.
   * If the table is sortable, can optionally add 'sortable' property to header
   * row object. If a particular column is not sortable, set to false.
   *
   * header = [
   *  { label: 'Username' },
   *  { label: 'Privilege' }
   *  { label: 'Account Status', sortable: false }
   * ]
   *
   * The 'sortable' attribute should be a boolean value. Defaults to false.
   * The 'default-sort' attribute should be the index value of the header
   * obejct that should be sorted on inital load.
   *
   * The 'row-actions-enabled' attribute, should be a boolean value
   * Can be set to true to render table row actions. Defaults to false.
   * Row actions are defined in data.actions.
   *
   * The 'expandable' attribute should be a boolean value. If true each
   * row object in data array should contain a 'expandContent' property
   *
   * The 'selectable' attribute should be a boolean value.
   * If 'selectable' is true, include 'batch-actions' property that should
   * be an array of actions to provide <table-toolbar> component.
   *
   * The 'size' attribute which can be set to 'small' which will
   * render a smaller font size in the table.
   *
   */

  const TableController = function() {
    this.sortAscending = true;
    this.activeSort;
    this.expandedRows = new Set();
    this.selectedRows = new Set();
    this.selectHeaderCheckbox = false;
    this.someSelected = false;

    let selectableRowCount = 0;

    /**
     * Sorts table data
     */
    const sortData = () => {
      this.data.sort((a, b) => {
        const aProp = a.uiData[this.activeSort];
        const bProp = b.uiData[this.activeSort];
        if (aProp === bProp) {
          return 0;
        } else {
          if (this.sortAscending) {
            return aProp < bProp ? -1 : 1;
          }
          return aProp > bProp ? -1 : 1;
        }
      })
    };

    /**
     * Prep table
     * Make adjustments to account for optional configurations
     */
    const prepTable = () => {
      if (this.sortable) {
        // If sort is enabled, check for undefined 'sortable'
        // property for each item in header array
        this.header = this.header.map((column) => {
          column.sortable =
              column.sortable === undefined ? true : column.sortable;
          return column;
        })
      }
    };

    /**
     * Prep data
     * When data binding changes, make adjustments to account for
     * optional configurations and undefined values
     */
    const prepData = () => {
      selectableRowCount = 0;
      this.data.forEach((row) => {
        if (row.uiData === undefined) {
          // Check for undefined 'uiData' property for each item in data
          // array
          row.uiData = [];
        }
        if (this.selectable) {
          // If table is selectable check row for 'selectable' property
          row.selectable = row.selectable === undefined ? true : row.selectable;
          if (row.selectable) {
            selectableRowCount++;
            row.selected = false;
          }
        }
      });
      if (this.sortable) {
        if (this.activeSort !== undefined || this.defaultSort !== undefined) {
          // apply default or active sort if one is defined
          this.activeSort = this.defaultSort !== undefined ? this.defaultSort :
                                                             this.activeSort;
          sortData();
        }
      }
    };

    /**
     * Select all rows
     * Sets each selectable row selected property to true
     * and adds index to selectedRow Set
     */
    const selectAllRows = () => {
      this.selectHeaderCheckbox = true;
      this.someSelected = false;
      this.data.forEach((row, index) => {
        if (!row.selected && row.selectable) {
          row.selected = true;
          this.selectedRows.add(index);
        }
      })
    };

    /**
     * Deselect all rows
     * Sets each row selected property to false
     * and clears selectedRow Set
     */
    const deselectAllRows = () => {
      this.selectHeaderCheckbox = false;
      this.someSelected = false;
      this.selectedRows.clear();
      this.data.forEach((row) => {
        if (row.selectable) {
          row.selected = false;
        }
      })
    };

    /**
     * Callback when table row action clicked
     * Emits user desired action and associated row data to
     * parent controller
     * @param {string} action : action type
     * @param {any} row : user object
     */
    this.onEmitRowAction = (action, row) => {
      if (action !== undefined && row !== undefined) {
        const value = {action, row};
        this.emitRowAction({value});
      }
    };

    /**
     * Callback when batch action clicked from toolbar
     * Emits the action type and the selected row data to
     * parent controller
     * @param {string} action : action type
     */
    this.onEmitBatchAction = (action) => {
      const filteredRows = this.data.filter((row) => row.selected);
      const value = {action, filteredRows};
      this.emitBatchAction({value});
    };

    /**
     * Callback when sortable table header clicked
     * @param {number} index : index of header item
     */
    this.onClickSort = (index) => {
      if (index === this.activeSort) {
        // If clicked header is already sorted, reverse
        // the sort direction
        this.sortAscending = !this.sortAscending;
        this.data.reverse();
      } else {
        this.sortAscending = true;
        this.activeSort = index;
        sortData();
      }
    };

    /**
     * Callback when expand trigger clicked
     * @param {number} row : index of expanded row
     */
    this.onClickExpand = (row) => {
      if (this.expandedRows.has(row)) {
        this.expandedRows.delete(row)
      } else {
        this.expandedRows.add(row);
      }
    };

    /**
     * Callback when select checkbox clicked
     * @param {number} row : index of selected row
     */
    this.onRowSelectChange = (row) => {
      if (this.selectedRows.has(row)) {
        this.selectedRows.delete(row);
      } else {
        this.selectedRows.add(row);
      }
      if (this.selectedRows.size === 0) {
        this.someSelected = false;
        this.selectHeaderCheckbox = false;
        deselectAllRows();
      } else if (this.selectedRows.size === selectableRowCount) {
        this.someSelected = false;
        this.selectHeaderCheckbox = true;
        selectAllRows();
      } else {
        this.someSelected = true;
      }
    };

    /**
     * Callback when header select box value changes
     */
    this.onHeaderSelectChange = (checked) => {
      this.selectHeaderCheckbox = checked;
      if (this.selectHeaderCheckbox) {
        selectAllRows();
      } else {
        deselectAllRows();
      }
    };

    /**
     * Callback when cancel/close button closed
     * from toolbar
     */
    this.onToolbarClose = () => {
      deselectAllRows();
    };

    /**
     * onInit Component lifecycle hook
     * Checking for undefined values
     */
    this.$onInit = () => {
      this.header = this.header === undefined ? [] : this.header;
      this.data = this.data == undefined ? [] : this.data;
      this.sortable = this.sortable === undefined ? false : this.sortable;
      this.rowActionsEnabled =
          this.rowActionsEnabled === undefined ? false : this.rowActionsEnabled;
      this.size = this.size === undefined ? '' : this.size;
      this.expandable = this.expandable === undefined ? false : this.expandable;
      this.selectable = this.selectable === undefined ? false : this.selectable;
      prepTable();
    };

    /**
     * onChanges Component lifecycle hook
     */
    this.$onChanges = (onChangesObj) => {
      const dataChange = onChangesObj.data;
      if (dataChange) {
        prepData();
        deselectAllRows();
      }
    };
  };

  /**
   * Register bmcTable component
   */
  angular.module('app.common.components').component('bmcTable', {
    template: require('./table.html'),
    controller: TableController,
    bindings: {
      data: '<',               // Array
      header: '<',             // Array
      rowActionsEnabled: '<',  // boolean
      size: '<',               // string
      sortable: '<',           // boolean
      defaultSort: '<',        // number (index of sort)
      expandable: '<',         // boolean
      selectable: '<',         // boolean
      batchActions: '<',       // Array
      emitRowAction: '&',
      emitBatchAction: '&'
    }
  })
})(window.angular);
