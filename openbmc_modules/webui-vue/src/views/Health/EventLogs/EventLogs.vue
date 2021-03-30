<template>
  <b-container fluid="xl">
    <page-title />
    <b-row class="mb-3">
      <b-col
        sm="8"
        md="7"
        xl="4"
        class="mb-2 mb-xl-0 d-flex flex-column justify-content-end"
      >
        <search
          :placeholder="$t('pageEventLogs.table.searchLogs')"
          @changeSearch="onChangeSearchInput"
        />
      </b-col>
      <b-col sm="8" md="7" xl="5" offset-xl="3">
        <table-date-filter @change="onChangeDateTimeFilter" />
      </b-col>
    </b-row>
    <b-row>
      <b-col class="text-right">
        <table-filter :filters="tableFilters" @filterChange="onFilterChange" />
      </b-col>
    </b-row>
    <b-row>
      <b-col>
        <table-toolbar
          ref="toolbar"
          :selected-items-count="selectedRows.length"
          :actions="batchActions"
          @clearSelected="clearSelectedRows($refs.table)"
          @batchAction="onBatchAction"
        >
          <template v-slot:export>
            <table-toolbar-export
              :data="batchExportData"
              :file-name="$t('appPageTitle.eventLogs')"
            />
          </template>
        </table-toolbar>
        <b-table
          id="table-event-logs"
          ref="table"
          responsive="md"
          selectable
          no-select-on-click
          sort-icon-left
          no-sort-reset
          sort-desc
          show-empty
          sort-by="date"
          :fields="fields"
          :items="filteredLogs"
          :sort-compare="onSortCompare"
          :empty-text="$t('pageEventLogs.table.emptyMessage')"
          :per-page="perPage"
          :current-page="currentPage"
          :filter="searchFilter"
          @row-selected="onRowSelected($event, filteredLogs.length)"
        >
          <!-- Checkbox column -->
          <template v-slot:head(checkbox)>
            <b-form-checkbox
              v-model="tableHeaderCheckboxModel"
              data-test-id="eventLogs-checkbox-selectAll"
              :indeterminate="tableHeaderCheckboxIndeterminate"
              @change="onChangeHeaderCheckbox($refs.table)"
            />
          </template>
          <template v-slot:cell(checkbox)="row">
            <b-form-checkbox
              v-model="row.rowSelected"
              :data-test-id="`eventLogs-checkbox-selectRow-${row.index}`"
              @change="toggleSelectRow($refs.table, row.index)"
            />
          </template>

          <!-- Severity column -->
          <template v-slot:cell(severity)="{ value }">
            <status-icon v-if="value" :status="statusIcon(value)" />
            {{ value }}
          </template>

          <!-- Date column -->
          <template v-slot:cell(date)="{ value }">
            {{ value | formatDate }} {{ value | formatTime }}
          </template>

          <!-- Actions column -->
          <template v-slot:cell(actions)="row">
            <table-row-action
              v-for="(action, index) in row.item.actions"
              :key="index"
              :value="action.value"
              :title="action.title"
              :row-data="row.item"
              :export-name="row.item.id"
              :data-test-id="`eventLogs-button-deleteRow-${row.index}`"
              @click:tableAction="onTableRowAction($event, row.item)"
            >
              <template v-slot:icon>
                <icon-export v-if="action.value === 'export'" />
                <icon-trashcan v-if="action.value === 'delete'" />
              </template>
            </table-row-action>
          </template>
        </b-table>
      </b-col>
    </b-row>

    <!-- Table pagination -->
    <b-row>
      <b-col class="d-md-flex justify-content-between">
        <b-form-group
          class="table-pagination-select"
          :label="$t('global.table.itemsPerPage')"
          label-for="pagination-items-per-page"
        >
          <b-form-select
            id="pagination-items-per-page"
            v-model="perPage"
            :options="itemsPerPageOptions"
          />
        </b-form-group>
        <b-pagination
          v-model="currentPage"
          first-number
          last-number
          :per-page="perPage"
          :total-rows="getTotalRowCount(filteredLogs.length)"
          aria-controls="table-event-logs"
        />
      </b-col>
    </b-row>
  </b-container>
</template>

<script>
import IconTrashcan from '@carbon/icons-vue/es/trash-can/20';
import IconExport from '@carbon/icons-vue/es/document--export/20';
import { omit } from 'lodash';

import PageTitle from '@/components/Global/PageTitle';
import StatusIcon from '@/components/Global/StatusIcon';
import Search from '@/components/Global/Search';
import TableDateFilter from '@/components/Global/TableDateFilter';
import TableFilter from '@/components/Global/TableFilter';
import TableRowAction from '@/components/Global/TableRowAction';
import TableToolbar from '@/components/Global/TableToolbar';
import TableToolbarExport from '@/components/Global/TableToolbarExport';

import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';
import TableFilterMixin from '@/components/Mixins/TableFilterMixin';
import BVPaginationMixin from '@/components/Mixins/BVPaginationMixin';
import BVTableSelectableMixin from '@/components/Mixins/BVTableSelectableMixin';
import BVToastMixin from '@/components/Mixins/BVToastMixin';
import TableDataFormatterMixin from '@/components/Mixins/TableDataFormatterMixin';
import TableSortMixin from '@/components/Mixins/TableSortMixin';

export default {
  components: {
    IconExport,
    IconTrashcan,
    PageTitle,
    Search,
    StatusIcon,
    TableFilter,
    TableRowAction,
    TableToolbar,
    TableToolbarExport,
    TableDateFilter
  },
  mixins: [
    BVPaginationMixin,
    BVTableSelectableMixin,
    BVToastMixin,
    LoadingBarMixin,
    TableFilterMixin,
    TableDataFormatterMixin,
    TableSortMixin
  ],
  data() {
    return {
      fields: [
        {
          key: 'checkbox',
          sortable: false
        },
        {
          key: 'id',
          label: this.$t('pageEventLogs.table.id'),
          sortable: true
        },
        {
          key: 'severity',
          label: this.$t('pageEventLogs.table.severity'),
          sortable: true
        },
        {
          key: 'type',
          label: this.$t('pageEventLogs.table.type'),
          sortable: true
        },
        {
          key: 'date',
          label: this.$t('pageEventLogs.table.date'),
          sortable: true
        },
        {
          key: 'description',
          label: this.$t('pageEventLogs.table.description')
        },
        {
          key: 'actions',
          sortable: false,
          label: '',
          tdClass: 'text-right text-nowrap'
        }
      ],
      tableFilters: [
        {
          key: 'severity',
          label: this.$t('pageEventLogs.table.severity'),
          values: ['OK', 'Warning', 'Critical']
        }
      ],
      activeFilters: [],
      batchActions: [
        {
          value: 'delete',
          label: this.$t('global.action.delete')
        }
      ],
      filterStartDate: null,
      filterEndDate: null,
      searchFilter: null
    };
  },
  computed: {
    allLogs() {
      return this.$store.getters['eventLog/allEvents'].map(event => {
        return {
          ...event,
          actions: [
            {
              value: 'export',
              title: this.$t('global.action.export')
            },
            {
              value: 'delete',
              title: this.$t('global.action.delete')
            }
          ]
        };
      });
    },
    batchExportData() {
      return this.selectedRows.map(row => omit(row, 'actions'));
    },
    filteredLogsByDate() {
      return this.getFilteredTableDataByDate(
        this.allLogs,
        this.filterStartDate,
        this.filterEndDate
      );
    },
    filteredLogs() {
      return this.getFilteredTableData(
        this.filteredLogsByDate,
        this.activeFilters
      );
    }
  },
  created() {
    this.startLoader();
    this.$store
      .dispatch('eventLog/getEventLogData')
      .finally(() => this.endLoader());
  },
  beforeRouteLeave(to, from, next) {
    // Hide loader if the user navigates to another page
    // before request is fulfilled.
    this.hideLoader();
    next();
  },
  methods: {
    deleteLogs(uris) {
      this.$store.dispatch('eventLog/deleteEventLogs', uris).then(messages => {
        messages.forEach(({ type, message }) => {
          if (type === 'success') {
            this.successToast(message);
          } else if (type === 'error') {
            this.errorToast(message);
          }
        });
      });
    },
    onFilterChange({ activeFilters }) {
      this.activeFilters = activeFilters;
    },
    onSortCompare(a, b, key) {
      if (key === 'severity') {
        return this.sortStatus(a, b, key);
      }
    },
    onTableRowAction(action, { uri }) {
      if (action === 'delete') {
        this.$bvModal
          .msgBoxConfirm(this.$tc('pageEventLogs.modal.deleteMessage'), {
            title: this.$tc('pageEventLogs.modal.deleteTitle'),
            okTitle: this.$t('global.action.delete')
          })
          .then(deleteConfirmed => {
            if (deleteConfirmed) this.deleteLogs([uri]);
          });
      }
    },
    onBatchAction(action) {
      if (action === 'delete') {
        const uris = this.selectedRows.map(row => row.uri);
        this.$bvModal
          .msgBoxConfirm(
            this.$tc(
              'pageEventLogs.modal.deleteMessage',
              this.selectedRows.length
            ),
            {
              title: this.$tc(
                'pageEventLogs.modal.deleteTitle',
                this.selectedRows.length
              ),
              okTitle: this.$t('global.action.delete')
            }
          )
          .then(deleteConfirmed => {
            if (deleteConfirmed) this.deleteLogs(uris);
          });
      }
    },
    onChangeDateTimeFilter({ fromDate, toDate }) {
      this.filterStartDate = fromDate;
      this.filterEndDate = toDate;
    },
    onChangeSearchInput(searchValue) {
      this.searchFilter = searchValue;
    }
  }
};
</script>
