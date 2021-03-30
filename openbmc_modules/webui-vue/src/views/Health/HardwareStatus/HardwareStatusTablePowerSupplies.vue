<template>
  <page-section :section-title="$t('pageHardwareStatus.powerSupplies')">
    <b-row>
      <b-col sm="6" md="5" xl="4">
        <search @changeSearch="onChangeSearchInput" />
      </b-col>
    </b-row>
    <b-table
      sort-icon-left
      no-sort-reset
      responsive="md"
      sort-by="health"
      :items="powerSupplies"
      :fields="fields"
      :sort-desc="true"
      :sort-compare="sortCompare"
      :filter="searchFilter"
    >
      <!-- Expand chevron icon -->
      <template v-slot:cell(expandRow)="row">
        <b-button
          variant="link"
          data-test-id="hardwareStatus-button-expandPowerSupplies"
          @click="row.toggleDetails"
        >
          <icon-chevron />
        </b-button>
      </template>

      <!-- Health -->
      <template v-slot:cell(health)="{ value }">
        <status-icon :status="statusIcon(value)" />
        {{ value }}
      </template>

      <template v-slot:row-details="{ item }">
        <b-container fluid>
          <b-row>
            <b-col sm="6" xl="4">
              <dl>
                <!-- Efficiency percent -->
                <dt>{{ $t('pageHardwareStatus.table.efficiencyPercent') }}:</dt>
                <dd>{{ tableFormatter(item.efficiencyPercent) }}</dd>
                <br />
                <!-- Firmware version -->
                <dt>{{ $t('pageHardwareStatus.table.firmwareVersion') }}:</dt>
                <dd>{{ tableFormatter(item.firmwareVersion) }}</dd>
                <br />
                <!-- Indicator LED -->
                <dt>{{ $t('pageHardwareStatus.table.indicatorLed') }}:</dt>
                <dd>{{ tableFormatter(item.indicatorLed) }}</dd>
              </dl>
            </b-col>
            <b-col sm="6" xl="4">
              <dl>
                <!-- Model -->
                <dt>{{ $t('pageHardwareStatus.table.model') }}:</dt>
                <dd>{{ tableFormatter(item.model) }}</dd>
                <br />
                <!-- Power input watts -->
                <dt>{{ $t('pageHardwareStatus.table.powerInputWatts') }}:</dt>
                <dd>{{ tableFormatter(item.powerInputWatts) }}</dd>
                <br />
                <!-- Status state -->
                <dt>{{ $t('pageHardwareStatus.table.statusState') }}:</dt>
                <dd>{{ tableFormatter(item.statusState) }}</dd>
              </dl>
            </b-col>
          </b-row>
        </b-container>
      </template>
    </b-table>
  </page-section>
</template>

<script>
import PageSection from '@/components/Global/PageSection';
import IconChevron from '@carbon/icons-vue/es/chevron--down/20';

import StatusIcon from '@/components/Global/StatusIcon';
import TableDataFormatterMixin from '@/components/Mixins/TableDataFormatterMixin';
import TableSortMixin from '@/components/Mixins/TableSortMixin';
import Search from '@/components/Global/Search';

export default {
  components: { IconChevron, PageSection, StatusIcon, Search },
  mixins: [TableDataFormatterMixin, TableSortMixin],
  data() {
    return {
      fields: [
        {
          key: 'expandRow',
          label: '',
          tdClass: 'table-row-expand',
          sortable: false
        },
        {
          key: 'id',
          label: this.$t('pageHardwareStatus.table.id'),
          formatter: this.tableFormatter,
          sortable: true
        },
        {
          key: 'health',
          label: this.$t('pageHardwareStatus.table.health'),
          formatter: this.tableFormatter,
          sortable: true
        },
        {
          key: 'partNumber',
          label: this.$t('pageHardwareStatus.table.partNumber'),
          formatter: this.tableFormatter,
          sortable: true
        },
        {
          key: 'serialNumber',
          label: this.$t('pageHardwareStatus.table.serialNumber'),
          formatter: this.tableFormatter,
          sortable: true
        }
      ],
      searchFilter: null
    };
  },
  computed: {
    powerSupplies() {
      return this.$store.getters['powerSupply/powerSupplies'];
    }
  },
  created() {
    this.$store.dispatch('powerSupply/getPowerSupply').finally(() => {
      // Emit initial data fetch complete to parent component
      this.$root.$emit('hardwareStatus::powerSupplies::complete');
    });
  },
  methods: {
    sortCompare(a, b, key) {
      if (key === 'health') {
        return this.sortStatus(a, b, key);
      }
    },
    onChangeSearchInput(searchValue) {
      this.searchFilter = searchValue;
    }
  }
};
</script>
