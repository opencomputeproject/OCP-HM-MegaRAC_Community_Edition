<template>
  <page-section :section-title="$t('pageHardwareStatus.fans')">
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
      :items="fans"
      :fields="fields"
      :sort-desc="true"
      :sort-compare="sortCompare"
      :filter="searchFilter"
    >
      <!-- Expand chevron icon -->
      <template v-slot:cell(expandRow)="row">
        <b-button
          variant="link"
          data-test-id="hardwareStatus-button-expandFans"
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
    fans() {
      return this.$store.getters['fan/fans'];
    }
  },
  created() {
    this.$store.dispatch('fan/getFanInfo').finally(() => {
      // Emit initial data fetch complete to parent component
      this.$root.$emit('hardwareStatus::fans::complete');
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
