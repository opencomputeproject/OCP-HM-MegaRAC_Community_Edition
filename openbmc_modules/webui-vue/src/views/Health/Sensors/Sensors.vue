<template>
  <b-container fluid="xl">
    <page-title />
    <b-row>
      <b-col md="5" xl="4">
        <search
          :placeholder="$t('pageSensors.searchForSensors')"
          @changeSearch="onChangeSearchInput"
        />
      </b-col>
      <b-col md="7" xl="8" class="text-right">
        <table-filter :filters="tableFilters" @filterChange="onFilterChange" />
      </b-col>
    </b-row>
    <b-row>
      <b-col xl="12">
        <table-toolbar
          ref="toolbar"
          :selected-items-count="selectedRows.length"
          @clearSelected="clearSelectedRows($refs.table)"
        >
          <template v-slot:export>
            <table-toolbar-export
              :data="selectedRows"
              :file-name="$t('appPageTitle.sensors')"
            />
          </template>
        </table-toolbar>
        <b-table
          ref="table"
          responsive="md"
          selectable
          no-select-on-click
          sort-icon-left
          no-sort-reset
          sticky-header="75vh"
          sort-by="status"
          :items="filteredSensors"
          :fields="fields"
          :sort-desc="true"
          :sort-compare="sortCompare"
          :filter="searchFilter"
          @row-selected="onRowSelected($event, filteredSensors.length)"
        >
          <!-- Checkbox column -->
          <template v-slot:head(checkbox)>
            <b-form-checkbox
              v-model="tableHeaderCheckboxModel"
              :indeterminate="tableHeaderCheckboxIndeterminate"
              @change="onChangeHeaderCheckbox($refs.table)"
            />
          </template>
          <template v-slot:cell(checkbox)="row">
            <b-form-checkbox
              v-model="row.rowSelected"
              @change="toggleSelectRow($refs.table, row.index)"
            />
          </template>

          <template v-slot:cell(status)="{ value }">
            <status-icon :status="statusIcon(value)" />
            {{ value }}
          </template>
          <template v-slot:cell(currentValue)="data">
            {{ data.value }} {{ data.item.units }}
          </template>
          <template v-slot:cell(lowerCaution)="data">
            {{ data.value }} {{ data.item.units }}
          </template>
          <template v-slot:cell(upperCaution)="data">
            {{ data.value }} {{ data.item.units }}
          </template>
          <template v-slot:cell(lowerCritical)="data">
            {{ data.value }} {{ data.item.units }}
          </template>
          <template v-slot:cell(upperCritical)="data">
            {{ data.value }} {{ data.item.units }}
          </template>
        </b-table>
      </b-col>
    </b-row>
  </b-container>
</template>

<script>
import PageTitle from '@/components/Global/PageTitle';
import Search from '@/components/Global/Search';
import StatusIcon from '@/components/Global/StatusIcon';
import TableFilter from '@/components/Global/TableFilter';
import TableToolbar from '@/components/Global/TableToolbar';
import TableToolbarExport from '@/components/Global/TableToolbarExport';

import BVTableSelectableMixin from '@/components/Mixins/BVTableSelectableMixin';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';
import TableFilterMixin from '@/components/Mixins/TableFilterMixin';
import TableDataFormatterMixin from '@/components/Mixins/TableDataFormatterMixin';
import TableSortMixin from '@/components/Mixins/TableSortMixin';

export default {
  name: 'Sensors',
  components: {
    PageTitle,
    Search,
    StatusIcon,
    TableFilter,
    TableToolbar,
    TableToolbarExport
  },
  mixins: [
    TableFilterMixin,
    BVTableSelectableMixin,
    LoadingBarMixin,
    TableDataFormatterMixin,
    TableSortMixin
  ],
  data() {
    return {
      fields: [
        {
          key: 'checkbox',
          sortable: false,
          label: ''
        },
        {
          key: 'name',
          sortable: true,
          label: this.$t('pageSensors.table.name')
        },
        {
          key: 'status',
          sortable: true,
          label: this.$t('pageSensors.table.status')
        },
        {
          key: 'lowerCritical',
          formatter: this.tableFormatter,
          label: this.$t('pageSensors.table.lowerCritical')
        },
        {
          key: 'lowerCaution',
          formatter: this.tableFormatter,
          label: this.$t('pageSensors.table.lowerWarning')
        },

        {
          key: 'currentValue',
          formatter: this.tableFormatter,
          label: this.$t('pageSensors.table.currentValue')
        },
        {
          key: 'upperCaution',
          formatter: this.tableFormatter,
          label: this.$t('pageSensors.table.upperWarning')
        },
        {
          key: 'upperCritical',
          formatter: this.tableFormatter,
          label: this.$t('pageSensors.table.upperCritical')
        }
      ],
      tableFilters: [
        {
          key: 'status',
          label: this.$t('pageSensors.table.status'),
          values: ['OK', 'Warning', 'Critical']
        }
      ],
      activeFilters: [],
      searchFilter: null
    };
  },
  computed: {
    allSensors() {
      return this.$store.getters['sensors/sensors'];
    },
    filteredSensors() {
      return this.getFilteredTableData(this.allSensors, this.activeFilters);
    }
  },
  created() {
    this.startLoader();
    this.$store
      .dispatch('sensors/getAllSensors')
      .finally(() => this.endLoader());
  },
  beforeRouteLeave(to, from, next) {
    this.hideLoader();
    next();
  },
  methods: {
    sortCompare(a, b, key) {
      if (key === 'status') {
        return this.sortStatus(a, b, key);
      }
    },
    onFilterChange({ activeFilters }) {
      this.activeFilters = activeFilters;
    },
    onChangeSearchInput(event) {
      this.searchFilter = event;
    }
  }
};
</script>
