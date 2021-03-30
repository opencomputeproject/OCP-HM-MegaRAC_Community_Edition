<template>
  <b-container fluid="xl">
    <page-title />
    <div class="quicklinks-section">
      <overview-quick-links />
    </div>
    <b-row>
      <b-col>
        <page-section :section-title="$t('pageOverview.bmcInformation')">
          <b-row>
            <b-col>
              <dl>
                <dt>{{ $t('pageOverview.firmwareVersion') }}</dt>
                <dd>{{ bmcFirmwareVersion }}</dd>
              </dl>
            </b-col>
          </b-row>
        </page-section>
        <b-row>
          <b-col>
            <page-section
              :section-title="$t('pageOverview.networkInformation')"
            >
              <overview-network />
            </page-section>
          </b-col>
        </b-row>
      </b-col>
      <b-col>
        <page-section :section-title="$t('pageOverview.serverInformation')">
          <b-row>
            <b-col sm="6">
              <dl>
                <dt>{{ $t('pageOverview.model') }}</dt>
                <dd>{{ serverModel }}</dd>
              </dl>
            </b-col>
            <b-col sm="6">
              <dl>
                <dt>{{ $t('pageOverview.manufacturer') }}</dt>
                <dd>{{ serverManufacturer }}</dd>
              </dl>
            </b-col>
            <b-col sm="6">
              <dl>
                <dt>{{ $t('pageOverview.serialNumber') }}</dt>
                <dd>{{ serverSerialNumber }}</dd>
              </dl>
            </b-col>
            <b-col sm="6">
              <dl>
                <dt>{{ $t('pageOverview.firmwareVersion') }}</dt>
                <dd>{{ hostFirmwareVersion }}</dd>
              </dl>
            </b-col>
          </b-row>
        </page-section>
        <page-section :section-title="$t('pageOverview.powerConsumption')">
          <b-row>
            <b-col sm="6">
              <dl>
                <dt>{{ $t('pageOverview.powerConsumption') }}</dt>
                <dd v-if="powerConsumptionValue == null">
                  {{ $t('global.status.notAvailable') }}
                </dd>
                <dd v-else>{{ powerConsumptionValue }} W</dd>
              </dl>
            </b-col>
            <b-col sm="6">
              <dl>
                <dt>{{ $t('pageOverview.powerCap') }}</dt>
                <dd v-if="powerCapValue == null">
                  {{ $t('global.status.disabled') }}
                </dd>
                <dd v-else>{{ powerCapValue }} W</dd>
              </dl>
            </b-col>
          </b-row>
        </page-section>
      </b-col>
    </b-row>
    <page-section :section-title="$t('pageOverview.highPriorityEvents')">
      <overview-events />
    </page-section>
  </b-container>
</template>

<script>
import OverviewQuickLinks from './OverviewQuickLinks';
import OverviewEvents from './OverviewEvents';
import OverviewNetwork from './OverviewNetwork';
import PageTitle from '../../components/Global/PageTitle';
import PageSection from '../../components/Global/PageSection';
import { mapState } from 'vuex';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';

export default {
  name: 'Overview',
  components: {
    OverviewQuickLinks,
    OverviewEvents,
    OverviewNetwork,
    PageTitle,
    PageSection
  },
  mixins: [LoadingBarMixin],
  computed: mapState({
    server: state => state.system.systems[0],
    bmcFirmwareVersion: state => state.firmware.bmcFirmwareVersion,
    powerCapValue: state => state.powerControl.powerCapValue,
    powerConsumptionValue: state => state.powerControl.powerConsumptionValue,
    serverManufacturer() {
      if (this.server) return this.server.manufacturer || '--';
      return '--';
    },
    serverModel() {
      if (this.server) return this.server.model || '--';
      return '--';
    },
    serverSerialNumber() {
      if (this.server) return this.server.serialNumber || '--';
      return '--';
    },
    hostFirmwareVersion() {
      if (this.server) return this.server.firmwareVersion || '--';
      return '--';
    }
  }),
  created() {
    this.startLoader();
    const quicklinksPromise = new Promise(resolve => {
      this.$root.$on('overview::quicklinks::complete', () => resolve());
    });
    const networkPromise = new Promise(resolve => {
      this.$root.$on('overview::network::complete', () => resolve());
    });
    const eventsPromise = new Promise(resolve => {
      this.$root.$on('overview::events::complete', () => resolve());
    });
    Promise.all([
      this.$store.dispatch('system/getSystem'),
      this.$store.dispatch('firmware/getBmcFirmware'),
      this.$store.dispatch('powerControl/getPowerControl'),
      quicklinksPromise,
      networkPromise,
      eventsPromise
    ]).finally(() => this.endLoader());
  },
  beforeRouteLeave(to, from, next) {
    this.hideLoader();
    next();
  }
};
</script>

<style lang="scss" scoped>
.quicklinks-section {
  margin-bottom: $spacer * 2;
  margin-left: $spacer * -1;
}

dd {
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
</style>
