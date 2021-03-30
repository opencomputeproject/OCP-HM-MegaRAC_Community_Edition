<template>
  <b-container fluid="xl">
    <page-title />
    <b-row>
      <b-col md="8" xl="6">
        <alert variant="info" class="mb-4">
          <span>
            {{ $t('pageDateTimeSettings.alert.message') }}
            <b-link to="/profile-settings">
              {{ $t('pageDateTimeSettings.alert.link') }}</b-link
            >
          </span>
        </alert>
      </b-col>
    </b-row>
    <page-section>
      <b-row>
        <b-col lg="3">
          <dl>
            <dt>{{ $t('pageDateTimeSettings.form.date') }}</dt>
            <dd v-if="bmcTime">{{ bmcTime | formatDate }}</dd>
            <dd v-else>--</dd>
          </dl>
        </b-col>
        <b-col lg="3">
          <dl>
            <dt>{{ $t('pageDateTimeSettings.form.time') }}</dt>
            <dd v-if="bmcTime">{{ bmcTime | formatTime }}</dd>
            <dd v-else>--</dd>
          </dl>
        </b-col>
      </b-row>
    </page-section>
    <page-section :section-title="$t('pageDateTimeSettings.configureSettings')">
      <b-form novalidate @submit.prevent="submitForm">
        <b-form-group label="Configure date and time" label-sr-only>
          <b-form-radio
            v-model="form.configurationSelected"
            value="manual"
            data-test-id="dateTimeSettings-radio-configureManual"
            @change="onChangeConfigType"
          >
            {{ $t('pageDateTimeSettings.form.manual') }}
          </b-form-radio>
          <b-row class="mt-3 ml-3">
            <b-col sm="6" lg="4" xl="3">
              <b-form-group
                :label="$t('pageDateTimeSettings.form.date')"
                label-for="input-manual-date"
              >
                <b-form-text id="date-format-help">(YYYY-MM-DD)</b-form-text>
                <b-input-group>
                  <b-form-input
                    id="input-manual-date"
                    v-model="form.manual.date"
                    :state="getValidationState($v.form.manual.date)"
                    :disabled="form.configurationSelected === 'ntp'"
                    data-test-id="dateTimeSettings-input-manualDate"
                    @blur="$v.form.manual.date.$touch()"
                  />
                  <b-form-invalid-feedback role="alert">
                    <div v-if="!$v.form.manual.date.pattern">
                      {{ $t('global.form.invalidFormat') }}
                    </div>
                    <div v-if="!$v.form.manual.date.required">
                      {{ $t('global.form.fieldRequired') }}
                    </div>
                  </b-form-invalid-feedback>
                  <b-form-datepicker
                    v-model="form.manual.date"
                    button-only
                    right
                    size="sm"
                    :hide-header="true"
                    :locale="locale"
                    :label-help="
                      $t('global.calendar.useCursorKeysToNavigateCalendarDates')
                    "
                    :disabled="form.configurationSelected === 'ntp'"
                    button-variant="link"
                    aria-controls="input-manual-date"
                  >
                    <template v-slot:button-content>
                      <icon-calendar />
                      <span class="sr-only">
                        {{ $t('global.calendar.openDatePicker') }}
                      </span>
                    </template>
                  </b-form-datepicker>
                </b-input-group>
              </b-form-group>
            </b-col>
            <b-col sm="6" lg="4" xl="3">
              <b-form-group
                :label="$t('pageDateTimeSettings.form.time')"
                label-for="input-manual-time"
              >
                <b-form-text id="time-format-help">(HH:MM)</b-form-text>
                <b-input-group>
                  <b-form-input
                    id="input-manual-time"
                    v-model="form.manual.time"
                    :state="getValidationState($v.form.manual.time)"
                    :disabled="form.configurationSelected === 'ntp'"
                    data-test-id="dateTimeSettings-input-manualTime"
                    @blur="$v.form.manual.time.$touch()"
                  />
                  <b-form-invalid-feedback role="alert">
                    <div v-if="!$v.form.manual.time.pattern">
                      {{ $t('global.form.invalidFormat') }}
                    </div>
                    <div v-if="!$v.form.manual.time.required">
                      {{ $t('global.form.fieldRequired') }}
                    </div>
                  </b-form-invalid-feedback>
                </b-input-group>
              </b-form-group>
            </b-col>
          </b-row>
          <b-form-radio
            v-model="form.configurationSelected"
            value="ntp"
            data-test-id="dateTimeSettings-radio-configureNTP"
            @change="onChangeConfigType"
          >
            NTP
          </b-form-radio>
          <b-row class="mt-3 ml-3">
            <b-col sm="6" lg="4" xl="3">
              <b-form-group
                :label="$t('pageDateTimeSettings.form.ntpServers.server1')"
                label-for="input-ntp-1"
              >
                <b-input-group>
                  <b-form-input
                    id="input-ntp-1"
                    v-model="form.ntp.firstAddress"
                    :state="getValidationState($v.form.ntp.firstAddress)"
                    :disabled="form.configurationSelected === 'manual'"
                    data-test-id="dateTimeSettings-input-ntpServer1"
                    @blur="$v.form.ntp.firstAddress.$touch()"
                  />
                  <b-form-invalid-feedback role="alert">
                    <div v-if="!$v.form.ntp.firstAddress.required">
                      {{ $t('global.form.fieldRequired') }}
                    </div>
                  </b-form-invalid-feedback>
                </b-input-group>
              </b-form-group>
            </b-col>
            <b-col sm="6" lg="4" xl="3">
              <b-form-group
                :label="$t('pageDateTimeSettings.form.ntpServers.server2')"
                label-for="input-ntp-2"
              >
                <b-input-group>
                  <b-form-input
                    id="input-ntp-2"
                    v-model="form.ntp.secondAddress"
                    :disabled="form.configurationSelected === 'manual'"
                    data-test-id="dateTimeSettings-input-ntpServer2"
                    @blur="$v.form.ntp.secondAddress.$touch()"
                  />
                </b-input-group>
              </b-form-group>
            </b-col>
            <b-col sm="6" lg="4" xl="3">
              <b-form-group
                :label="$t('pageDateTimeSettings.form.ntpServers.server3')"
                label-for="input-ntp-3"
              >
                <b-input-group>
                  <b-form-input
                    id="input-ntp-3"
                    v-model="form.ntp.thirdAddress"
                    :disabled="form.configurationSelected === 'manual'"
                    data-test-id="dateTimeSettings-input-ntpServer3"
                    @blur="$v.form.ntp.thirdAddress.$touch()"
                  />
                </b-input-group>
              </b-form-group>
            </b-col>
          </b-row>
        </b-form-group>
        <b-button
          variant="primary"
          type="submit"
          data-test-id="dateTimeSettings-button-saveSettings"
        >
          {{ $t('global.action.saveSettings') }}
        </b-button>
      </b-form>
    </page-section>
  </b-container>
</template>

<script>
import Alert from '@/components/Global/Alert';
import IconCalendar from '@carbon/icons-vue/es/calendar/20';
import PageTitle from '@/components/Global/PageTitle';
import PageSection from '@/components/Global/PageSection';

import BVToastMixin from '@/components/Mixins/BVToastMixin';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';
import VuelidateMixin from '@/components/Mixins/VuelidateMixin.js';

import { mapState } from 'vuex';
import { requiredIf, helpers } from 'vuelidate/lib/validators';

const isoDateRegex = /([12]\d{3}-(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01]))/;
const isoTimeRegex = /^(0[0-9]|1[0-9]|2[0-3]):[0-5][0-9]$/;

export default {
  name: 'DateTimeSettings',
  components: { Alert, IconCalendar, PageTitle, PageSection },
  mixins: [BVToastMixin, LoadingBarMixin, VuelidateMixin],
  data() {
    return {
      locale: this.$store.getters['global/languagePreference'],
      form: {
        configurationSelected: '',
        manual: {
          date: '',
          time: ''
        },
        ntp: { firstAddress: '', secondAddress: '', thirdAddress: '' }
      }
    };
  },
  validations() {
    return {
      form: {
        manual: {
          date: {
            required: requiredIf(function() {
              return this.form.configurationSelected === 'manual';
            }),
            pattern: helpers.regex('pattern', isoDateRegex)
          },
          time: {
            required: requiredIf(function() {
              return this.form.configurationSelected === 'manual';
            }),
            pattern: helpers.regex('pattern', isoTimeRegex)
          }
        },
        ntp: {
          firstAddress: {
            required: requiredIf(function() {
              return this.form.configurationSelected === 'ntp';
            })
          },
          secondAddress: {},
          thirdAddress: {}
        }
      }
    };
  },
  computed: {
    ...mapState('dateTime', ['ntpServers', 'isNtpProtocolEnabled']),
    bmcTime() {
      return this.$store.getters['global/bmcTime'];
    }
  },
  watch: {
    ntpServers() {
      this.setNtpValues();
    },
    manualDate() {
      this.emitChange();
    }
  },
  created() {
    this.startLoader();
    Promise.all([
      this.$store.dispatch('global/getBmcTime'),
      this.$store.dispatch('dateTime/getNtpData')
    ]).finally(() => this.endLoader());
  },
  beforeRouteLeave(to, from, next) {
    this.hideLoader();
    next();
  },
  methods: {
    emitChange() {
      if (this.$v.$invalid) return;
      this.$v.$reset(); //reset to re-validate on blur
      this.$emit('change', {
        manualDate: this.manualDate ? new Date(this.manualDate) : null
      });
    },
    setNtpValues() {
      this.form.configurationSelected = this.isNtpProtocolEnabled
        ? 'ntp'
        : 'manual';
      this.form.ntp.firstAddress = this.ntpServers[0] || '';
      this.form.ntp.secondAddress = this.ntpServers[1] || '';
      this.form.ntp.thirdAddress = this.ntpServers[2] || '';
    },
    onChangeConfigType() {
      this.$v.form.$reset();
      this.setNtpValues();
    },
    submitForm() {
      this.$v.$touch();
      if (this.$v.$invalid) return;
      this.startLoader();

      let dateTimeForm = {};
      let ntpFirstAddress;
      let ntpSecondAddress;
      let ntpThirdAddress;
      let isNTPEnabled = this.form.configurationSelected === 'ntp';

      if (!isNTPEnabled) {
        dateTimeForm.ntpProtocolEnabled = false;
        dateTimeForm.updatedDateTime = new Date(
          `${this.form.manual.date} ${this.form.manual.time}`
        ).toISOString();
      } else {
        ntpFirstAddress = this.form.ntp.firstAddress;
        ntpSecondAddress = this.form.ntp.secondAddress;
        ntpThirdAddress = this.form.ntp.thirdAddress;
        dateTimeForm.ntpProtocolEnabled = true;
        dateTimeForm.ntpServersArray = [
          ntpFirstAddress,
          ntpSecondAddress,
          ntpThirdAddress
        ];
      }

      this.$store
        .dispatch('dateTime/updateDateTimeSettings', dateTimeForm)
        .then(success => {
          this.successToast(success);
          if (!isNTPEnabled) return;
          // Shift address up if second address is empty
          // to avoid refreshing after delay when updating NTP
          if (ntpSecondAddress === '' && ntpThirdAddress !== '') {
            this.form.ntp.secondAddress = ntpThirdAddress;
            this.form.ntp.thirdAddress = '';
          }
        })
        .then(() => {
          this.$store.dispatch('global/getBmcTime');
        })
        .catch(({ message }) => this.errorToast(message))
        .finally(() => {
          this.$v.form.$reset();
          this.endLoader();
        });
    }
  }
};
</script>

<style lang="scss" scoped>
@import 'src/assets/styles/helpers';

.b-form-datepicker {
  position: absolute;
  right: 0;
  top: 0;
  z-index: $zindex-dropdown + 1;
}
</style>
