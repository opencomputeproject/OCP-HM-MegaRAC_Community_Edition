<template>
  <div class="quicklinks form-background">
    <div>
      <dl>
        <dt>{{ $t('pageOverview.quicklinks.bmcTime') }}</dt>
        <dd v-if="bmcTime">
          {{ bmcTime | formatDate }} {{ bmcTime | formatTime }}
        </dd>
        <dd v-else>--</dd>
      </dl>
    </div>
    <div>
      <dl>
        <dt>{{ $t('pageOverview.quicklinks.serverLed') }}</dt>
        <dd>
          <b-form-checkbox
            v-model="serverLedChecked"
            data-test-id="overviewQuickLinks-checkbox-serverLed"
            name="check-button"
            switch
            value="Lit"
            unchecked-value="Off"
            @change="onChangeServerLed"
          >
            <span v-if="serverLedChecked !== 'Off'">
              {{ $t('global.status.on') }}
            </span>
            <span v-else>{{ $t('global.status.off') }}</span>
          </b-form-checkbox>
        </dd>
      </dl>
    </div>
    <div>
      <b-button
        to="/configuration/network-settings"
        variant="secondary"
        data-test-id="overviewQuickLinks-button-networkSettings"
        class="d-flex justify-content-between align-items-center"
      >
        <span>{{ $t('pageOverview.quicklinks.editNetworkSettings') }}</span>
        <icon-arrow-right />
      </b-button>
    </div>
    <div>
      <b-button
        to="/control/serial-over-lan"
        variant="secondary"
        data-test-id="overviewQuickLinks-button-solConsole"
        class="d-flex justify-content-between align-items-center"
      >
        <span>{{ $t('pageOverview.quicklinks.solConsole') }}</span>
        <icon-arrow-right />
      </b-button>
    </div>
  </div>
</template>

<script>
import ArrowRight16 from '@carbon/icons-vue/es/arrow--right/16';
import BVToastMixin from '@/components/Mixins/BVToastMixin';

export default {
  name: 'QuickLinks',
  components: {
    IconArrowRight: ArrowRight16
  },
  mixins: [BVToastMixin],
  computed: {
    bmcTime() {
      return this.$store.getters['global/bmcTime'];
    },
    serverLedChecked: {
      get() {
        return this.$store.getters['serverLed/getIndicatorValue'];
      },
      set(value) {
        return value;
      }
    }
  },
  created() {
    Promise.all([
      this.$store.dispatch('global/getBmcTime'),
      this.$store.dispatch('serverLed/getIndicatorValue')
    ]).finally(() => {
      this.$root.$emit('overview::quicklinks::complete');
    });
  },
  methods: {
    onChangeServerLed(value) {
      this.$store
        .dispatch('serverLed/saveIndicatorLedValue', value)
        .then(message => this.successToast(message))
        .catch(({ message }) => this.errorToast(message));
    }
  }
};
</script>

<style lang="scss" scoped>
dd,
dl {
  margin: 0;
}

.quicklinks {
  display: grid;
  grid-gap: 1rem;
  padding: 1rem;
  white-space: nowrap;
  align-items: center;
}

@include media-breakpoint-up(sm) {
  .quicklinks {
    grid-template-columns: repeat(2, 1fr);
  }
}

@include media-breakpoint-up(xl) {
  .quicklinks {
    grid-template-columns: repeat(4, 1fr);
  }
}
</style>
