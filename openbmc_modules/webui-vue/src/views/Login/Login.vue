<template>
  <b-form
    class="login-form  mx-auto ml-md-5 mb-3"
    novalidate
    @submit.prevent="login"
  >
    <alert class="login-error mb-4" :show="authError" variant="danger">
      <p id="login-error-alert">
        {{ $t('pageLogin.alert.message') }}
      </p>
    </alert>
    <b-form-group label-for="language" :label="$t('pageLogin.language')">
      <b-form-select
        id="language"
        v-model="$i18n.locale"
        :options="languages"
        data-test-id="login-select-language"
      ></b-form-select>
    </b-form-group>
    <b-form-group label-for="username" :label="$t('pageLogin.username')">
      <b-form-input
        id="username"
        v-model="userInfo.username"
        aria-describedby="login-error-alert username-required"
        :state="getValidationState($v.userInfo.username)"
        type="text"
        autofocus="autofocus"
        data-test-id="login-input-username"
        @input="$v.userInfo.username.$touch()"
      >
      </b-form-input>
      <b-form-invalid-feedback id="username-required" role="alert">
        <template v-if="!$v.userInfo.username.required">
          {{ $t('global.form.fieldRequired') }}
        </template>
      </b-form-invalid-feedback>
    </b-form-group>
    <div class="login-form__section mb-3">
      <label for="password">{{ $t('pageLogin.password') }}</label>
      <b-form-input
        id="password"
        v-model="userInfo.password"
        aria-describedby="login-error-alert password-required"
        :state="getValidationState($v.userInfo.password)"
        type="password"
        data-test-id="login-input-password"
        @input="$v.userInfo.password.$touch()"
      >
      </b-form-input>
      <b-form-invalid-feedback id="password-required" role="alert">
        <template v-if="!$v.userInfo.password.required">
          {{ $t('global.form.fieldRequired') }}
        </template>
      </b-form-invalid-feedback>
    </div>
    <b-button
      block
      class="mt-5"
      type="submit"
      variant="primary"
      data-test-id="login-button-submit"
      :disabled="disableSubmitButton"
      >{{ $t('pageLogin.logIn') }}</b-button
    >
  </b-form>
</template>

<script>
import { required } from 'vuelidate/lib/validators';
import VuelidateMixin from '../../components/Mixins/VuelidateMixin.js';
import i18n from '../../i18n';
import Alert from '../../components/Global/Alert';

export default {
  name: 'Login',
  components: { Alert },
  mixins: [VuelidateMixin],
  data() {
    return {
      userInfo: {
        username: null,
        password: null
      },
      disableSubmitButton: false,
      languages: [
        {
          value: 'en-US',
          text: 'English'
        },
        {
          value: 'es',
          text: 'Español'
        }
      ]
    };
  },
  computed: {
    authError() {
      return this.$store.getters['authentication/authError'];
    }
  },
  validations: {
    userInfo: {
      username: {
        required
      },
      password: {
        required
      }
    }
  },
  methods: {
    login: function() {
      this.$v.$touch();
      if (this.$v.$invalid) return;
      this.disableSubmitButton = true;
      const username = this.userInfo.username;
      const password = this.userInfo.password;
      this.$store
        .dispatch('authentication/login', [username, password])
        .then(() => {
          localStorage.setItem('storedLanguage', i18n.locale);
          localStorage.setItem('storedUsername', username);
          this.$store.commit('global/setUsername', username);
          this.$store.commit('global/setLanguagePreference', i18n.locale);
          return this.$store.dispatch(
            'authentication/checkPasswordChangeRequired',
            username
          );
        })
        .then(passwordChangeRequired => {
          if (passwordChangeRequired) {
            this.$router.push('/change-password');
          } else {
            this.$router.push('/');
          }
        })
        .catch(error => console.log(error))
        .finally(() => (this.disableSubmitButton = false));
    }
  }
};
</script>

<style lang="scss" scoped>
.login-form {
  max-width: 360px;
}
</style>
