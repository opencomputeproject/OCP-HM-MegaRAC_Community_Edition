<template>
  <page-section :section-title="$t('pageHardwareStatus.system')">
    <b-table responsive="md" :items="systems" :fields="fields">
      <!-- Expand chevron icon -->
      <template v-slot:cell(expandRow)="row">
        <b-button
          variant="link"
          data-test-id="hardwareStatus-button-expandSystem"
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
                <!-- Asset tag -->
                <dt>{{ $t('pageHardwareStatus.table.assetTag') }}:</dt>
                <dd>{{ tableFormatter(item.assetTag) }}</dd>
                <br />
                <!-- Description -->
                <dt>{{ $t('pageHardwareStatus.table.description') }}:</dt>
                <dd>{{ tableFormatter(item.description) }}</dd>
                <br />
                <!-- Indicator LED -->
                <dt>{{ $t('pageHardwareStatus.table.indicatorLed') }}:</dt>
                <dd>{{ tableFormatter(item.indicatorLed) }}</dd>
                <br />
                <!-- Model -->
                <dt>{{ $t('pageHardwareStatus.table.model') }}:</dt>
                <dd>{{ tableFormatter(item.model) }}</dd>
              </dl>
            </b-col>
            <b-col sm="6" xl="4">
              <dl>
                <!-- Power state -->
                <dt>{{ $t('pageHardwareStatus.table.powerState') }}:</dt>
                <dd>{{ tableFormatter(item.powerState) }}</dd>
                <br />
                <!-- Health rollup -->
                <dt>
                  {{ $t('pageHardwareStatus.table.statusHealthRollup') }}:
                </dt>
                <dd>{{ tableFormatter(item.healthRollup) }}</dd>
                <br />
                <!-- Status state -->
                <dt>{{ $t('pageHardwareStatus.table.statusState') }}:</dt>
                <dd>{{ tableFormatter(item.statusState) }}</dd>
                <br />
                <!-- System type -->
                <dt>{{ $t('pageHardwareStatus.table.systemType') }}:</dt>
                <dd>{{ tableFormatter(item.systemType) }}</dd>
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

export default {
  components: { IconChevron, PageSection, StatusIcon },
  mixins: [TableDataFormatterMixin],
  data() {
    return {
      fields: [
        {
          key: 'expandRow',
          label: '',
          tdClass: 'table-row-expand'
        },
        {
          key: 'id',
          label: this.$t('pageHardwareStatus.table.id'),
          formatter: this.tableFormatter
        },
        {
          key: 'health',
          label: this.$t('pageHardwareStatus.table.health'),
          formatter: this.tableFormatter
        },
        {
          key: 'partNumber',
          label: this.$t('pageHardwareStatus.table.partNumber'),
          formatter: this.tableFormatter
        },
        {
          key: 'serialNumber',
          label: this.$t('pageHardwareStatus.table.serialNumber'),
          formatter: this.tableFormatter
        }
      ]
    };
  },
  computed: {
    systems() {
      return this.$store.getters['system/systems'];
    }
  },
  created() {
    this.$store.dispatch('system/getSystem').finally(() => {
      // Emit initial data fetch complete to parent component
      this.$root.$emit('hardwareStatus::system::complete');
    });
  }
};
</script>
