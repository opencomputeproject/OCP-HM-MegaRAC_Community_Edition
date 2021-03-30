<template>
  <b-row class="mb-2">
    <b-col class="d-flex">
      <b-form-group
        :label="$t('global.table.fromDate')"
        label-for="input-from-date"
        class="mr-3 my-0 w-100"
      >
        <b-input-group>
          <b-form-input
            id="input-from-date"
            v-model="fromDate"
            placeholder="YYYY-MM-DD"
            :state="getValidationState($v.fromDate)"
            @blur="$v.fromDate.$touch()"
          />
          <b-form-invalid-feedback role="alert">
            <template v-if="!$v.fromDate.pattern">
              {{ $t('global.form.invalidFormat') }}
            </template>
            <template v-if="!$v.fromDate.maxDate">
              {{ $t('global.form.dateMustBeBefore', { date: toDate }) }}
            </template>
          </b-form-invalid-feedback>
          <template slot:append>
            <b-form-datepicker
              v-model="fromDate"
              button-only
              right
              size="sm"
              :max="toDate"
              :hide-header="true"
              :locale="locale"
              :label-help="
                $t('global.calendar.useCursorKeysToNavigateCalendarDates')
              "
              button-variant="link"
              aria-controls="input-from-date"
            >
              <template v-slot:button-content>
                <icon-calendar />
                <span class="sr-only">{{
                  $t('global.calendar.openDatePicker')
                }}</span>
              </template>
            </b-form-datepicker>
          </template>
        </b-input-group>
      </b-form-group>
      <b-form-group
        :label="$t('global.table.toDate')"
        label-for="input-to-date"
        class="my-0 w-100"
      >
        <b-input-group>
          <b-form-input
            id="input-to-date"
            v-model="toDate"
            placeholder="YYYY-MM-DD"
            :state="getValidationState($v.toDate)"
            @blur="$v.toDate.$touch()"
          />
          <b-form-invalid-feedback role="alert">
            <template v-if="!$v.toDate.pattern">
              {{ $t('global.form.invalidFormat') }}
            </template>
            <template v-if="!$v.toDate.minDate">
              {{ $t('global.form.dateMustBeAfter', { date: fromDate }) }}
            </template>
          </b-form-invalid-feedback>
          <template slot:append>
            <b-form-datepicker
              v-model="toDate"
              button-only
              right
              size="sm"
              :min="fromDate"
              :hide-header="true"
              :locale="locale"
              :label-help="
                $t('global.calendar.useCursorKeysToNavigateCalendarDates')
              "
              button-variant="link"
              aria-controls="input-to-date"
            >
              <template v-slot:button-content>
                <icon-calendar />
                <span class="sr-only">{{
                  $t('global.calendar.openDatePicker')
                }}</span>
              </template>
            </b-form-datepicker>
          </template>
        </b-input-group>
      </b-form-group>
    </b-col>
  </b-row>
</template>

<script>
import IconCalendar from '@carbon/icons-vue/es/calendar/20';
import { helpers } from 'vuelidate/lib/validators';

import VuelidateMixin from '@/components/Mixins/VuelidateMixin.js';

const isoDateRegex = /([12]\d{3}-(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01]))/;

export default {
  components: { IconCalendar },
  mixins: [VuelidateMixin],
  data() {
    return {
      fromDate: '',
      toDate: '',
      offsetToDate: '',
      locale: this.$store.getters['global/languagePreference']
    };
  },
  validations() {
    return {
      fromDate: {
        pattern: helpers.regex('pattern', isoDateRegex),
        maxDate: value => {
          if (!this.toDate) return true;
          const date = new Date(value);
          const maxDate = new Date(this.toDate);
          if (date.getTime() > maxDate.getTime()) return false;
          return true;
        }
      },
      toDate: {
        pattern: helpers.regex('pattern', isoDateRegex),
        minDate: value => {
          if (!this.fromDate) return true;
          const date = new Date(value);
          const minDate = new Date(this.fromDate);
          if (date.getTime() < minDate.getTime()) return false;
          return true;
        }
      }
    };
  },
  watch: {
    fromDate() {
      this.emitChange();
    },
    toDate(newVal) {
      // Offset the end date to end of day to make sure all
      // entries from selected end date are included in filter
      this.offsetToDate = new Date(newVal).setUTCHours(23, 59, 59, 999);
      this.emitChange();
    }
  },
  methods: {
    emitChange() {
      if (this.$v.$invalid) return;
      this.$v.$reset(); //reset to re-validate on blur
      this.$emit('change', {
        fromDate: this.fromDate ? new Date(this.fromDate) : null,
        toDate: this.toDate ? new Date(this.offsetToDate) : null
      });
    }
  }
};
</script>

<style lang="scss" scoped>
.b-form-datepicker {
  position: absolute;
  right: 0;
  top: 0;
  z-index: $zindex-dropdown + 1;
}
</style>
