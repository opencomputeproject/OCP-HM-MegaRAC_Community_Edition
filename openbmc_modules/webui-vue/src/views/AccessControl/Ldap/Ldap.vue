<template>
  <b-container fluid="xl">
    <page-title :description="$t('pageLdap.pageDescription')" />
    <page-section :section-title="$t('pageLdap.settings')">
      <b-form novalidate @submit.prevent="handleSubmit">
        <b-row>
          <b-col>
            <b-form-group
              class="mb-3"
              :label="$t('pageLdap.form.ldapAuthentication')"
              label-for="enable-ldap-auth"
            >
              <b-form-checkbox
                id="enable-ldap-auth"
                v-model="form.ldapAuthenticationEnabled"
                data-test-id="ldap-checkbox-ldapAuthenticationEnabled"
                @change="onChangeldapAuthenticationEnabled"
              >
                {{ $t('global.action.enable') }}
              </b-form-checkbox>
            </b-form-group>
          </b-col>
        </b-row>
        <div class="form-background p-3">
          <b-form-group
            class="m-0"
            :label="$t('pageLdap.ariaLabel.ldapSettings')"
            label-class="sr-only"
            :disabled="!form.ldapAuthenticationEnabled"
          >
            <b-row>
              <b-col md="3" lg="4" xl="3">
                <b-form-group
                  class="mb-4"
                  :label="$t('pageLdap.form.secureLdapUsingSsl')"
                >
                  <b-form-text id="enable-secure-help-block">
                    {{ $t('pageLdap.form.secureLdapHelper') }}
                  </b-form-text>
                  <b-form-checkbox
                    id="enable-secure-ldap"
                    v-model="form.secureLdapEnabled"
                    aria-describedby="enable-secure-help-block"
                    data-test-id="ldap-checkbox-secureLdapEnabled"
                    :disabled="
                      !caCertificateExpiration || !ldapCertificateExpiration
                    "
                    @change="$v.form.secureLdapEnabled.$touch()"
                  >
                    {{ $t('global.action.enable') }}
                  </b-form-checkbox>
                </b-form-group>
                <dl>
                  <dt>{{ $t('pageLdap.form.caCertificateValidUntil') }}</dt>
                  <dd v-if="caCertificateExpiration">
                    {{ caCertificateExpiration | formatDate }}
                  </dd>
                  <dd v-else>--</dd>
                  <dt>{{ $t('pageLdap.form.ldapCertificateValidUntil') }}</dt>
                  <dd v-if="ldapCertificateExpiration">
                    {{ ldapCertificateExpiration | formatDate }}
                  </dd>
                  <dd v-else>--</dd>
                </dl>
                <b-link
                  class="d-inline-block mb-4 m-md-0"
                  to="/access-control/ssl-certificates"
                >
                  {{ $t('pageLdap.form.manageSslCertificates') }}
                </b-link>
              </b-col>
              <b-col md="9" lg="8" xl="9">
                <b-row>
                  <b-col>
                    <b-form-group :label="$t('pageLdap.form.serviceType')">
                      <b-form-radio
                        v-model="form.activeDirectoryEnabled"
                        data-test-id="ldap-radio-activeDirectoryEnabled"
                        :value="false"
                        @change="onChangeServiceType"
                      >
                        OpenLDAP
                      </b-form-radio>
                      <b-form-radio
                        v-model="form.activeDirectoryEnabled"
                        data-test-id="ldap-radio-activeDirectoryEnabled"
                        :value="true"
                        @change="onChangeServiceType"
                      >
                        Active Directory
                      </b-form-radio>
                    </b-form-group>
                  </b-col>
                </b-row>
                <b-row>
                  <b-col sm="6" xl="4">
                    <b-form-group label-for="server-uri">
                      <template v-slot:label>
                        {{ $t('pageLdap.form.serverUri') }}
                        <info-tooltip
                          :title="$t('pageLdap.form.serverUriTooltip')"
                        />
                      </template>
                      <b-input-group :prepend="ldapProtocol">
                        <b-form-input
                          id="server-uri"
                          v-model="form.serverUri"
                          data-test-id="ldap-input-serverUri"
                          :state="getValidationState($v.form.serverUri)"
                          @change="$v.form.serverUri.$touch()"
                        />
                        <b-form-invalid-feedback role="alert">
                          {{ $t('global.form.fieldRequired') }}
                        </b-form-invalid-feedback>
                      </b-input-group>
                    </b-form-group>
                  </b-col>
                  <b-col sm="6" xl="4">
                    <b-form-group
                      :label="$t('pageLdap.form.bindDn')"
                      label-for="bind-dn"
                    >
                      <b-form-input
                        id="bind-dn"
                        v-model="form.bindDn"
                        data-test-id="ldap-input-bindDn"
                        :state="getValidationState($v.form.bindDn)"
                        @change="$v.form.bindDn.$touch()"
                      />
                      <b-form-invalid-feedback role="alert">
                        {{ $t('global.form.fieldRequired') }}
                      </b-form-invalid-feedback>
                    </b-form-group>
                  </b-col>
                  <b-col sm="6" xl="4">
                    <b-form-group
                      :label="$t('pageLdap.form.bindPassword')"
                      label-for="bind-password"
                    >
                      <input-password-toggle
                        data-test-id="ldap-input-togglePassword"
                      >
                        <b-form-input
                          id="bind-password"
                          v-model="form.bindPassword"
                          type="password"
                          :state="getValidationState($v.form.bindPassword)"
                          @change="$v.form.bindPassword.$touch()"
                        />
                        <b-form-invalid-feedback role="alert">
                          {{ $t('global.form.fieldRequired') }}
                        </b-form-invalid-feedback>
                      </input-password-toggle>
                    </b-form-group>
                  </b-col>
                  <b-col sm="6" xl="4">
                    <b-form-group
                      :label="$t('pageLdap.form.baseDn')"
                      label-for="base-dn"
                    >
                      <b-form-input
                        id="base-dn"
                        v-model="form.baseDn"
                        data-test-id="ldap-input-baseDn"
                        :state="getValidationState($v.form.baseDn)"
                        @change="$v.form.baseDn.$touch()"
                      />
                      <b-form-invalid-feedback role="alert">
                        {{ $t('global.form.fieldRequired') }}
                      </b-form-invalid-feedback>
                    </b-form-group>
                  </b-col>
                  <b-col sm="6" xl="4">
                    <b-form-group
                      :label="$t('pageLdap.form.userIdAttribute')"
                      label-for="user-id-attribute"
                    >
                      <b-form-input
                        id="user-id-attribute"
                        v-model="form.userIdAttribute"
                        data-test-id="ldap-input-userIdAttribute"
                        @change="$v.form.userIdAttribute.$touch()"
                      />
                    </b-form-group>
                  </b-col>
                  <b-col sm="6" xl="4">
                    <b-form-group
                      :label="$t('pageLdap.form.groupIdAttribute')"
                      label-for="group-id-attribute"
                    >
                      <b-form-input
                        id="group-id-attribute"
                        v-model="form.groupIdAttribute"
                        data-test-id="ldap-input-groupIdAttribute"
                        @change="$v.form.groupIdAttribute.$touch()"
                      />
                    </b-form-group>
                  </b-col>
                </b-row>
              </b-col>
            </b-row>
          </b-form-group>
        </div>
        <b-row class="mt-4 mb-5">
          <b-col>
            <b-btn
              variant="primary"
              type="submit"
              data-test-id="ldap-button-saveSettings"
              :disabled="!$v.form.$anyDirty"
            >
              {{ $t('global.action.saveSettings') }}
            </b-btn>
          </b-col>
        </b-row>
      </b-form>
    </page-section>

    <!-- Role groups -->
    <page-section :section-title="$t('pageLdap.roleGroups')">
      <table-role-groups />
    </page-section>
  </b-container>
</template>

<script>
import { mapGetters } from 'vuex';
import { find } from 'lodash';
import { requiredIf } from 'vuelidate/lib/validators';

import BVToastMixin from '@/components/Mixins/BVToastMixin';
import VuelidateMixin from '@/components/Mixins/VuelidateMixin';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';
import InputPasswordToggle from '@/components/Global/InputPasswordToggle';
import PageTitle from '@/components/Global/PageTitle';
import PageSection from '@/components/Global/PageSection';
import InfoTooltip from '@/components/Global/InfoTooltip';
import TableRoleGroups from './TableRoleGroups';

export default {
  name: 'Ldap',
  components: {
    InfoTooltip,
    InputPasswordToggle,
    PageTitle,
    PageSection,
    TableRoleGroups
  },
  mixins: [BVToastMixin, VuelidateMixin, LoadingBarMixin],
  data() {
    return {
      form: {
        ldapAuthenticationEnabled: this.$store.getters['ldap/isServiceEnabled'],
        secureLdapEnabled: false,
        activeDirectoryEnabled: this.$store.getters[
          'ldap/isActiveDirectoryEnabled'
        ],
        serverUri: '',
        bindDn: '',
        bindPassword: '',
        baseDn: '',
        userIdAttribute: '',
        groupIdAttribute: ''
      }
    };
  },
  computed: {
    ...mapGetters('ldap', [
      'isServiceEnabled',
      'isActiveDirectoryEnabled',
      'ldap',
      'activeDirectory'
    ]),
    sslCertificates() {
      return this.$store.getters['sslCertificates/allCertificates'];
    },
    caCertificateExpiration() {
      const caCertificate = find(this.sslCertificates, {
        type: 'TrustStore Certificate'
      });
      if (caCertificate === undefined) return null;
      return caCertificate.validUntil;
    },
    ldapCertificateExpiration() {
      const ldapCertificate = find(this.sslCertificates, {
        type: 'LDAP Certificate'
      });
      if (ldapCertificate === undefined) return null;
      return ldapCertificate.validUntil;
    },
    ldapProtocol() {
      return this.form.secureLdapEnabled ? 'ldaps://' : 'ldap://';
    }
  },
  watch: {
    isServiceEnabled: function(value) {
      this.form.ldapAuthenticationEnabled = value;
    },
    isActiveDirectoryEnabled: function(value) {
      this.form.activeDirectoryEnabled = value;
      this.setFormValues();
    }
  },
  validations: {
    form: {
      ldapAuthenticationEnabled: {},
      secureLdapEnabled: {},
      activeDirectoryEnabled: {
        required: requiredIf(function() {
          return this.form.ldapAuthenticationEnabled;
        })
      },
      serverUri: {
        required: requiredIf(function() {
          return this.form.ldapAuthenticationEnabled;
        })
      },
      bindDn: {
        required: requiredIf(function() {
          return this.form.ldapAuthenticationEnabled;
        })
      },
      bindPassword: {
        required: requiredIf(function() {
          return this.form.ldapAuthenticationEnabled;
        })
      },
      baseDn: {
        required: requiredIf(function() {
          return this.form.ldapAuthenticationEnabled;
        })
      },
      userIdAttribute: {},
      groupIdAttribute: {}
    }
  },
  created() {
    this.startLoader();
    this.$store
      .dispatch('ldap/getAccountSettings')
      .finally(() => this.endLoader());
    this.$store.dispatch('sslCertificates/getCertificates');
    this.setFormValues();
  },
  beforeRouteLeave(to, from, next) {
    this.hideLoader();
    next();
  },
  methods: {
    setFormValues(serviceType) {
      if (!serviceType) {
        serviceType = this.isActiveDirectoryEnabled
          ? this.activeDirectory
          : this.ldap;
      }
      const {
        serviceAddress = '',
        bindDn = '',
        baseDn = '',
        userAttribute = '',
        groupsAttribute = ''
      } = serviceType;
      const secureLdap =
        serviceAddress && serviceAddress.includes('ldaps://') ? true : false;
      const serverUri = serviceAddress
        ? serviceAddress.replace(/ldaps?:\/\//, '')
        : '';
      this.form.secureLdapEnabled = secureLdap;
      this.form.serverUri = serverUri;
      this.form.bindDn = bindDn;
      this.form.bindPassword = '';
      this.form.baseDn = baseDn;
      this.form.userIdAttribute = userAttribute;
      this.form.groupIdAttribute = groupsAttribute;
    },
    handleSubmit() {
      this.$v.$touch();
      if (this.$v.$invalid) return;
      const data = {
        serviceEnabled: this.form.ldapAuthenticationEnabled,
        activeDirectoryEnabled: this.form.activeDirectoryEnabled,
        serviceAddress: `${this.ldapProtocol}${this.form.serverUri}`,
        bindDn: this.form.bindDn,
        bindPassword: this.form.bindPassword,
        baseDn: this.form.baseDn,
        userIdAttribute: this.form.userIdAttribute,
        groupIdAttribute: this.form.groupIdAttribute
      };
      this.startLoader();
      this.$store
        .dispatch('ldap/saveAccountSettings', data)
        .then(success => {
          this.successToast(success);
          this.$v.form.$reset();
        })
        .catch(({ message }) => this.errorToast(message))
        .finally(() => {
          this.form.bindPassword = '';
          this.endLoader();
        });
    },
    onChangeServiceType(isActiveDirectoryEnabled) {
      this.$v.form.activeDirectoryEnabled.$touch();
      const serviceType = isActiveDirectoryEnabled
        ? this.activeDirectory
        : this.ldap;
      // Set form values according to user selected
      // service type
      this.setFormValues(serviceType);
    },
    onChangeldapAuthenticationEnabled(isServiceEnabled) {
      this.$v.form.ldapAuthenticationEnabled.$touch();
      if (!isServiceEnabled) {
        // Request will fail if sent with empty values.
        // The frontend only checks for required fields
        // when the service is enabled. This is to prevent
        // an error if a user clears any properties then
        // disables the service.
        this.setFormValues();
      }
    }
  }
};
</script>
