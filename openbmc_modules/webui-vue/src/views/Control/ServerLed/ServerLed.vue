<template>
  <b-container fluid="xl">
    <page-title />
    <b-row>
      <b-col md="12">
        <page-section :section-title="$t('pageServerLed.serverLedTitle')">
          <b-form-group :label="$t('pageServerLed.serverLedSubTitle')">
            <b-form-checkbox
              v-model="indicatorLed"
              data-test-id="serverLed-checkbox-switchIndicatorLed"
              name="check-button"
              value="Lit"
              unchecked-value="Off"
              switch
              @change="changeLedValue"
            >
              <span v-if="indicatorLed && indicatorLed !== 'Off'">
                {{ $t('global.status.on') }}
              </span>
              <span v-else>
                {{ $t('global.status.off') }}
              </span>
            </b-form-checkbox>
          </b-form-group>
        </page-section>
      </b-col>
    </b-row>
  </b-container>
</template>

<script>
import PageTitle from '@/components/Global/PageTitle';
import PageSection from '@/components/Global/PageSection';
import BVToastMixin from '@/components/Mixins/BVToastMixin';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';

export default {
  name: 'ServerLed',
  components: { PageTitle, PageSection },
  mixins: [LoadingBarMixin, BVToastMixin],
  computed: {
    indicatorLed: {
      get() {
        return this.$store.getters['serverLed/getIndicatorValue'];
      },
      set(newValue) {
        return newValue;
      }
    }
  },
  created() {
    this.startLoader();
    this.$store
      .dispatch('serverLed/getIndicatorValue')
      .finally(() => this.endLoader());
  },
  beforeRouteLeave(to, from, next) {
    this.hideLoader();
    next();
  },
  methods: {
    changeLedValue(indicatorLed) {
      this.$store
        .dispatch('serverLed/saveIndicatorLedValue', indicatorLed)
        .then(message => this.successToast(message))
        .catch(({ message }) => {
          this.errorToast(message);
          if (indicatorLed === 'Off') {
            this.indicatorLed === 'Lit';
          } else {
            this.indicatorLed === 'Off';
          }
        });
    }
  }
};
</script>
