<template>
  <b-container fluid="xl">
    <page-title :description="$t('pageManagePowerUsage.description')" />

    <b-row>
      <b-col sm="8" md="6" xl="12">
        <dl>
          <dt>{{ $t('pageManagePowerUsage.powerConsumption') }}</dt>
          <dd>
            {{
              powerConsumptionValue
                ? `${powerConsumptionValue} W`
                : $t('global.status.notAvailable')
            }}
          </dd>
        </dl>
      </b-col>
    </b-row>

    <b-form @submit.prevent="submitForm">
      <b-row>
        <b-col sm="8" md="6" xl="12">
          <b-form-group
            :label="$t('pageManagePowerUsage.powerCapSettingLabel')"
          >
            <b-form-checkbox
              v-model="isPowerCapFieldEnabled"
              data-test-id="managePowerUsage-checkbox-togglePowerCapField"
              name="power-cap-setting"
            >
              {{ $t('pageManagePowerUsage.powerCapSettingData') }}
            </b-form-checkbox>
          </b-form-group>
        </b-col>
      </b-row>

      <b-row>
        <b-col sm="8" md="6" xl="3">
          <b-form-group
            id="input-group-1"
            :label="$t('pageManagePowerUsage.powerCapLabel')"
            label-for="input-1"
          >
            <b-form-text id="power-help-text">
              {{
                $t('pageManagePowerUsage.powerCapLabelTextInfo', {
                  min: 1,
                  max: 10000
                })
              }}
            </b-form-text>

            <b-form-input
              id="input-1"
              v-model.number="powerCapValue"
              :disabled="!isPowerCapFieldEnabled"
              data-test-id="managePowerUsage-input-powerCapValue"
              type="number"
              aria-describedby="power-help-text"
              :state="getValidationState($v.powerCapValue)"
            ></b-form-input>

            <b-form-invalid-feedback id="input-live-feedback" role="alert">
              <template v-if="!$v.powerCapValue.required">
                {{ $t('global.form.fieldRequired') }}
              </template>
              <template v-else-if="!$v.powerCapValue.between">
                {{ $t('global.form.invalidValue') }}
              </template>
            </b-form-invalid-feedback>
          </b-form-group>
        </b-col>
      </b-row>

      <b-button
        variant="primary"
        type="submit"
        data-test-id="managePowerUsage-button-savePowerCapValue"
      >
        {{ $t('global.action.save') }}
      </b-button>
    </b-form>
  </b-container>
</template>

<script>
import PageTitle from '@/components/Global/PageTitle';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';
import VuelidateMixin from '@/components/Mixins/VuelidateMixin.js';
import BVToastMixin from '@/components/Mixins/BVToastMixin';
import { requiredIf, between } from 'vuelidate/lib/validators';
import { mapGetters } from 'vuex';

export default {
  name: 'ManagePowerUsage',
  components: { PageTitle },
  mixins: [VuelidateMixin, BVToastMixin, LoadingBarMixin],
  computed: {
    ...mapGetters({
      powerConsumptionValue: 'powerControl/powerConsumptionValue'
    }),

    /**
      Computed property isPowerCapFieldEnabled is used to enable or disable the input field.
      The input field is enabled when the powercapValue property is not null.
   **/
    isPowerCapFieldEnabled: {
      get() {
        return this.powerCapValue !== null;
      },
      set(value) {
        let newValue = value ? '' : null;
        this.$v.$reset();
        this.$store.dispatch('powerControl/setPowerCapUpdatedValue', newValue);
      }
    },
    powerCapValue: {
      get() {
        return this.$store.getters['powerControl/powerCapValue'];
      },
      set(value) {
        this.$v.$touch();
        this.$store.dispatch('powerControl/setPowerCapUpdatedValue', value);
      }
    }
  },
  created() {
    this.startLoader();
    this.$store
      .dispatch('powerControl/getPowerControl')
      .finally(() => this.endLoader());
  },
  beforeRouteLeave(to, from, next) {
    this.hideLoader();
    next();
  },
  validations: {
    powerCapValue: {
      between: between(1, 10000),
      required: requiredIf(function() {
        return this.isPowerCapFieldEnabled;
      })
    }
  },
  methods: {
    submitForm() {
      this.$v.$touch();
      if (this.$v.$invalid) return;
      this.startLoader();
      this.$store
        .dispatch('powerControl/setPowerControl', this.powerCapValue)
        .then(message => this.successToast(message))
        .catch(({ message }) => this.errorToast(message))
        .finally(() => this.endLoader());
    }
  }
};
</script>
