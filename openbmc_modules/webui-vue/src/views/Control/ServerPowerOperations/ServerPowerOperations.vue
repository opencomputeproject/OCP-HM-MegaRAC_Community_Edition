<template>
  <b-container fluid="xl">
    <page-title />
    <b-row class="mb-4">
      <b-col md="8" xl="6">
        <page-section
          :section-title="$t('pageServerPowerOperations.currentStatus')"
        >
          <dl>
            <dt>{{ $t('pageServerPowerOperations.hostStatus') }}</dt>
            <dd v-if="hostStatus === 'on'">
              {{ $t('global.status.on') }}
            </dd>
            <dd v-else-if="hostStatus === 'off'">
              {{ $t('global.status.off') }}
            </dd>
            <dd v-else>
              {{ $t('global.status.notAvailable') }}
            </dd>
          </dl>
        </page-section>
      </b-col>
    </b-row>
    <b-row>
      <b-col sm="8" md="6" xl="4">
        <page-section
          :section-title="$t('pageServerPowerOperations.hostOsBootSettings')"
        >
          <boot-settings />
        </page-section>
      </b-col>
      <b-col sm="8" md="6" xl="7">
        <page-section
          :section-title="$t('pageServerPowerOperations.operations')"
        >
          <b-alert :show="oneTimeBootEnabled" variant="warning">
            {{ $t('pageServerPowerOperations.oneTimeBootWarning') }}
          </b-alert>
          <template v-if="isOperationInProgress">
            {{ $t('pageServerPowerOperations.operationInProgress') }}
          </template>
          <template v-else-if="hostStatus === 'off'">
            <b-button
              variant="primary"
              data-test-id="serverPowerOperations-button-powerOn"
              @click="powerOn"
            >
              {{ $t('pageServerPowerOperations.powerOn') }}
            </b-button>
          </template>
          <template v-else-if="hostStatus === 'on'">
            <!-- Reboot server options -->
            <b-form novalidate class="mb-5" @submit.prevent="rebootServer">
              <b-form-group
                :label="$t('pageServerPowerOperations.rebootServer')"
              >
                <b-form-radio
                  v-model="form.rebootOption"
                  name="reboot-option"
                  data-test-id="serverPowerOperations-radio-rebootOrderly"
                  value="orderly"
                >
                  {{ $t('pageServerPowerOperations.orderlyReboot') }}
                </b-form-radio>
                <b-form-radio
                  v-model="form.rebootOption"
                  name="reboot-option"
                  data-test-id="serverPowerOperations-radio-rebootImmediate"
                  value="immediate"
                >
                  {{ $t('pageServerPowerOperations.immediateReboot') }}
                </b-form-radio>
              </b-form-group>
              <b-button
                variant="primary"
                type="submit"
                data-test-id="serverPowerOperations-button-reboot"
              >
                {{ $t('pageServerPowerOperations.reboot') }}
              </b-button>
            </b-form>
            <!-- Shutdown server options -->
            <b-form novalidate @submit.prevent="shutdownServer">
              <b-form-group
                :label="$t('pageServerPowerOperations.shutdownServer')"
              >
                <b-form-radio
                  v-model="form.shutdownOption"
                  name="shutdown-option"
                  data-test-id="serverPowerOperations-radio-shutdownOrderly"
                  value="orderly"
                >
                  {{ $t('pageServerPowerOperations.orderlyShutdown') }}
                </b-form-radio>
                <b-form-radio
                  v-model="form.shutdownOption"
                  name="shutdown-option"
                  data-test-id="serverPowerOperations-radio-shutdownImmediate"
                  value="immediate"
                >
                  {{ $t('pageServerPowerOperations.immediateShutdown') }}
                </b-form-radio>
              </b-form-group>
              <b-button
                variant="primary"
                type="submit"
                data-test-id="serverPowerOperations-button-shutDown"
              >
                {{ $t('pageServerPowerOperations.shutDown') }}
              </b-button>
            </b-form>
          </template>
          <template v-else>
            {{ $t('global.status.notAvailable') }}
          </template>
        </page-section>
      </b-col>
    </b-row>
  </b-container>
</template>

<script>
import PageTitle from '../../../components/Global/PageTitle';
import PageSection from '../../../components/Global/PageSection';
import BVToastMixin from '../../../components/Mixins/BVToastMixin';
import BootSettings from './BootSettings';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';

export default {
  name: 'ServerPowerOperations',
  components: { PageTitle, PageSection, BootSettings },
  mixins: [BVToastMixin, LoadingBarMixin],
  data() {
    return {
      form: {
        rebootOption: 'orderly',
        shutdownOption: 'orderly'
      }
    };
  },
  computed: {
    hostStatus() {
      return this.$store.getters['global/hostStatus'];
    },
    isOperationInProgress() {
      return this.$store.getters['controls/isOperationInProgress'];
    },
    oneTimeBootEnabled() {
      return this.$store.getters['hostBootSettings/overrideEnabled'];
    }
  },
  created() {
    this.startLoader();
  },
  beforeRouteLeave(to, from, next) {
    this.hideLoader();
    next();
  },
  methods: {
    powerOn() {
      this.$store.dispatch('controls/hostPowerOn');
    },
    rebootServer() {
      const modalMessage = this.$t(
        'pageServerPowerOperations.modal.confirmRebootMessage'
      );
      const modalOptions = {
        title: this.$t('pageServerPowerOperations.modal.confirmRebootTitle'),
        okTitle: this.$t('global.action.confirm')
      };

      if (this.form.rebootOption === 'orderly') {
        this.$bvModal
          .msgBoxConfirm(modalMessage, modalOptions)
          .then(confirmed => {
            if (confirmed) this.$store.dispatch('controls/hostSoftReboot');
          });
      } else if (this.form.rebootOption === 'immediate') {
        this.$bvModal
          .msgBoxConfirm(modalMessage, modalOptions)
          .then(confirmed => {
            if (confirmed) this.$store.dispatch('controls/hostHardReboot');
          });
      }
    },
    shutdownServer() {
      const modalMessage = this.$t(
        'pageServerPowerOperations.modal.confirmShutdownMessage'
      );
      const modalOptions = {
        title: this.$t('pageServerPowerOperations.modal.confirmShutdownTitle'),
        okTitle: this.$t('global.action.confirm')
      };

      if (this.form.shutdownOption === 'orderly') {
        this.$bvModal
          .msgBoxConfirm(modalMessage, modalOptions)
          .then(confirmed => {
            if (confirmed) this.$store.dispatch('controls/hostSoftPowerOff');
          });
      }
      if (this.form.shutdownOption === 'immediate') {
        this.$bvModal
          .msgBoxConfirm(modalMessage, modalOptions)
          .then(confirmed => {
            if (confirmed) this.$store.dispatch('controls/hostHardPowerOff');
          });
      }
    }
  }
};
</script>
