<template>
  <b-container fluid="xl">
    <page-title />

    <b-row>
      <b-col md="8" lg="8" xl="6">
        <page-section
          :section-title="$t('pageProfileSettings.profileInfoTitle')"
        >
          <dl>
            <dt>{{ $t('pageProfileSettings.username') }}</dt>
            <dd>
              {{ username }}
            </dd>
          </dl>
        </page-section>
      </b-col>
    </b-row>

    <b-form @submit.prevent="submitForm">
      <b-row>
        <b-col sm="8" md="6" xl="3">
          <page-section
            :section-title="$t('pageProfileSettings.changePassword')"
          >
            <b-form-group
              id="input-group-1"
              :label="$t('pageProfileSettings.newPassword')"
              label-for="input-1"
            >
              <b-form-text id="password-help-block">
                {{
                  $t('pageLocalUserManagement.modal.passwordMustBeBetween', {
                    min: passwordRequirements.minLength,
                    max: passwordRequirements.maxLength
                  })
                }}
              </b-form-text>
              <input-password-toggle>
                <b-form-input
                  id="password"
                  v-model="form.newPassword"
                  type="password"
                  aria-describedby="password-help-block"
                  :state="getValidationState($v.form.newPassword)"
                  @input="$v.form.newPassword.$touch()"
                />
                <b-form-invalid-feedback role="alert">
                  <template
                    v-if="
                      !$v.form.newPassword.minLength ||
                        !$v.form.newPassword.maxLength
                    "
                  >
                    {{
                      $t('pageProfileSettings.newPassLabelTextInfo', {
                        min: passwordRequirements.minLength,
                        max: passwordRequirements.maxLength
                      })
                    }}
                  </template>
                </b-form-invalid-feedback>
              </input-password-toggle>
            </b-form-group>
            <b-form-group
              id="input-group-2"
              :label="$t('pageProfileSettings.confirmPassword')"
              label-for="input-2"
            >
              <input-password-toggle>
                <b-form-input
                  id="password-confirmation"
                  v-model="form.confirmPassword"
                  type="password"
                  :state="getValidationState($v.form.confirmPassword)"
                  @input="$v.form.confirmPassword.$touch()"
                />
                <b-form-invalid-feedback role="alert">
                  <template v-if="!$v.form.confirmPassword.sameAsPassword">
                    {{ $t('pageProfileSettings.passwordsDoNotMatch') }}
                  </template>
                </b-form-invalid-feedback>
              </input-password-toggle>
            </b-form-group>
          </page-section>
        </b-col>
      </b-row>
      <page-section :section-title="$t('pageProfileSettings.timezoneDisplay')">
        <p>{{ $t('pageProfileSettings.timezoneDisplayDesc') }}</p>
        <b-row>
          <b-col md="9" lg="8" xl="9">
            <b-form-group :label="$t('pageProfileSettings.timezone')">
              <b-form-radio
                v-model="form.isUtcDisplay"
                :value="true"
                @change="$v.form.isUtcDisplay.$touch()"
              >
                {{ $t('pageProfileSettings.defaultUTC') }}
              </b-form-radio>
              <b-form-radio
                v-model="form.isUtcDisplay"
                :value="false"
                @change="$v.form.isUtcDisplay.$touch()"
              >
                {{
                  $t('pageProfileSettings.browserOffset', {
                    timezone
                  })
                }}
              </b-form-radio>
            </b-form-group>
          </b-col>
        </b-row>
      </page-section>
      <b-button variant="primary" type="submit">
        {{ $t('global.action.saveSettings') }}
      </b-button>
    </b-form>
  </b-container>
</template>

<script>
import i18n from '@/i18n';
import BVToastMixin from '@/components/Mixins/BVToastMixin';
import InputPasswordToggle from '@/components/Global/InputPasswordToggle';
import { format } from 'date-fns-tz';
import {
  maxLength,
  minLength,
  required,
  sameAs
} from 'vuelidate/lib/validators';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';
import PageTitle from '@/components/Global/PageTitle';
import PageSection from '@/components/Global/PageSection';
import VuelidateMixin from '@/components/Mixins/VuelidateMixin.js';

export default {
  name: 'ProfileSettings',
  components: { InputPasswordToggle, PageSection, PageTitle },
  mixins: [BVToastMixin, LoadingBarMixin, VuelidateMixin],
  data() {
    return {
      form: {
        newPassword: '',
        confirmPassword: '',
        isUtcDisplay: this.$store.getters['global/isUtcDisplay']
      }
    };
  },
  computed: {
    username() {
      return this.$store.getters['global/username'];
    },
    passwordRequirements() {
      return this.$store.getters['localUsers/accountPasswordRequirements'];
    },
    timezone() {
      const timezone = Intl.DateTimeFormat().resolvedOptions().timeZone;
      const shortTz = this.$options.filters.shortTimeZone(new Date());
      const pattern = `'${shortTz}' O`;
      return format(new Date(), pattern, { timezone }).replace('GMT', 'UTC');
    }
  },
  created() {
    this.startLoader();
    this.$store
      .dispatch('localUsers/getAccountSettings')
      .finally(() => this.endLoader());
  },
  validations() {
    return {
      form: {
        isUtcDisplay: { required },
        newPassword: {
          minLength: minLength(this.passwordRequirements.minLength),
          maxLength: maxLength(this.passwordRequirements.maxLength)
        },
        confirmPassword: {
          sameAsPassword: sameAs('newPassword')
        }
      }
    };
  },
  methods: {
    saveNewPasswordInputData() {
      this.$v.form.confirmPassword.$touch();
      this.$v.form.newPassword.$touch();
      if (this.$v.$invalid) return;
      let userData = {
        originalUsername: this.username,
        password: this.form.newPassword
      };

      this.$store
        .dispatch('localUsers/updateUser', userData)
        .then(message => {
          (this.form.newPassword = ''), (this.form.confirmPassword = '');
          this.$v.$reset();
          this.successToast(message);
        })
        .catch(({ message }) => this.errorToast(message));
    },
    saveTimeZonePrefrenceData() {
      localStorage.setItem('storedUtcDisplay', this.form.isUtcDisplay);
      this.$store.commit('global/setUtcTime', this.form.isUtcDisplay);
      this.successToast(
        i18n.t('pageProfileSettings.toast.successSaveSettings')
      );
    },
    submitForm() {
      if (this.form.confirmPassword || this.form.newPassword) {
        this.saveNewPasswordInputData();
      }
      if (this.$v.form.isUtcDisplay.$anyDirty) {
        this.saveTimeZonePrefrenceData();
      }
    }
  }
};
</script>
