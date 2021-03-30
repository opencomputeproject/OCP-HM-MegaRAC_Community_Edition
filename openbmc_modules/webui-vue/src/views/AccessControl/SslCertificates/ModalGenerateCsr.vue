<template>
  <div>
    <b-modal
      id="generate-csr"
      ref="modal"
      size="lg"
      no-stacking
      :title="
        $t('pageSslCertificates.modal.generateACertificateSigningRequest')
      "
      @ok="onOkGenerateCsrModal"
      @cancel="resetForm"
      @hidden="$v.$reset()"
    >
      <b-form id="generate-csr-form" novalidate @submit.prevent="handleSubmit">
        <b-container fluid>
          <b-row>
            <b-col lg="9">
              <b-row>
                <b-col lg="6">
                  <b-form-group
                    :label="
                      $t('pageSslCertificates.modal.certificateType') + ' *'
                    "
                    label-for="certificate-type"
                  >
                    <b-form-select
                      id="certificate-type"
                      v-model="form.certificateType"
                      data-test-id="modalGenerateCsr-select-certificateType"
                      :options="certificateOptions"
                      :state="getValidationState($v.form.certificateType)"
                      @input="$v.form.certificateType.$touch()"
                    >
                      <template v-slot:first>
                        <b-form-select-option :value="null" disabled>
                          {{ $t('global.form.selectAnOption') }}
                        </b-form-select-option>
                      </template>
                    </b-form-select>
                    <b-form-invalid-feedback role="alert">
                      {{ $t('global.form.fieldRequired') }}
                    </b-form-invalid-feedback>
                  </b-form-group>
                </b-col>
                <b-col lg="6">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.country') + ' *'"
                    label-for="country"
                  >
                    <b-form-select
                      id="country"
                      v-model="form.country"
                      data-test-id="modalGenerateCsr-select-country"
                      :options="countryOptions"
                      :state="getValidationState($v.form.country)"
                      @input="$v.form.country.$touch()"
                    >
                      <template v-slot:first>
                        <b-form-select-option :value="null" disabled>
                          {{ $t('global.form.selectAnOption') }}
                        </b-form-select-option>
                      </template>
                    </b-form-select>
                    <b-form-invalid-feedback role="alert">
                      {{ $t('global.form.fieldRequired') }}
                    </b-form-invalid-feedback>
                  </b-form-group>
                </b-col>
              </b-row>
              <b-row>
                <b-col lg="6">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.state') + ' *'"
                    label-for="state"
                  >
                    <b-form-input
                      id="state"
                      v-model="form.state"
                      type="text"
                      data-test-id="modalGenerateCsr-input-state"
                      :state="getValidationState($v.form.state)"
                    />
                    <b-form-invalid-feedback role="alert">
                      {{ $t('global.form.fieldRequired') }}
                    </b-form-invalid-feedback>
                  </b-form-group>
                </b-col>
                <b-col lg="6">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.city') + ' *'"
                    label-for="city"
                  >
                    <b-form-input
                      id="city"
                      v-model="form.city"
                      type="text"
                      data-test-id="modalGenerateCsr-input-city"
                      :state="getValidationState($v.form.city)"
                    />
                    <b-form-invalid-feedback role="alert">
                      {{ $t('global.form.fieldRequired') }}
                    </b-form-invalid-feedback>
                  </b-form-group>
                </b-col>
              </b-row>
              <b-row>
                <b-col lg="6">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.companyName') + ' *'"
                    label-for="company-name"
                  >
                    <b-form-input
                      id="company-name"
                      v-model="form.companyName"
                      type="text"
                      data-test-id="modalGenerateCsr-input-companyName"
                      :state="getValidationState($v.form.companyName)"
                    />
                    <b-form-invalid-feedback role="alert">
                      {{ $t('global.form.fieldRequired') }}
                    </b-form-invalid-feedback>
                  </b-form-group>
                </b-col>
                <b-col lg="6">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.companyUnit') + ' *'"
                    label-for="company-unit"
                  >
                    <b-form-input
                      id="company-unit"
                      v-model="form.companyUnit"
                      type="text"
                      data-test-id="modalGenerateCsr-input-companyUnit"
                      :state="getValidationState($v.form.companyUnit)"
                    />
                    <b-form-invalid-feedback role="alert">
                      {{ $t('global.form.fieldRequired') }}
                    </b-form-invalid-feedback>
                  </b-form-group>
                </b-col>
              </b-row>
              <b-row>
                <b-col lg="6">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.commonName') + ' *'"
                    label-for="common-name"
                  >
                    <b-form-input
                      id="common-name"
                      v-model="form.commonName"
                      type="text"
                      data-test-id="modalGenerateCsr-input-commonName"
                      :state="getValidationState($v.form.commonName)"
                    />
                    <b-form-invalid-feedback role="alert">
                      {{ $t('global.form.fieldRequired') }}
                    </b-form-invalid-feedback>
                  </b-form-group>
                </b-col>
                <b-col lg="6">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.challengePassword')"
                    label-for="challenge-password"
                  >
                    <b-form-input
                      id="challenge-password"
                      v-model="form.challengePassword"
                      type="text"
                      data-test-id="modalGenerateCsr-input-challengePassword"
                    />
                  </b-form-group>
                </b-col>
              </b-row>
              <b-row>
                <b-col lg="6">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.contactPerson')"
                    label-for="contact-person"
                  >
                    <b-form-input
                      id="contact-person"
                      v-model="form.contactPerson"
                      type="text"
                      data-test-id="modalGenerateCsr-input-contactPerson"
                    />
                  </b-form-group>
                </b-col>
                <b-col lg="6">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.emailAddress')"
                    label-for="email-address"
                  >
                    <b-form-input
                      id="email-address"
                      v-model="form.emailAddress"
                      type="text"
                      data-test-id="modalGenerateCsr-input-emailAddress"
                    />
                  </b-form-group>
                </b-col>
              </b-row>
              <b-row>
                <b-col lg="12">
                  <b-form-group
                    :label="$t('pageSslCertificates.modal.alternateName')"
                    label-for="alternate-name"
                  >
                    <b-form-text id="alternate-name-help-block">
                      {{
                        $t('pageSslCertificates.modal.alternateNameHelperText')
                      }}
                    </b-form-text>
                    <b-form-tags
                      v-model="form.alternateName"
                      :remove-on-delete="true"
                      :tag-pills="true"
                      input-id="alternate-name"
                      size="lg"
                      separator=" "
                      :input-attrs="{
                        'aria-describedby': 'alternate-name-help-block'
                      }"
                      :duplicate-tag-text="
                        $t('pageSslCertificates.modal.duplicateAlternateName')
                      "
                      placeholder=""
                    >
                      <template v-slot:add-button-text>
                        {{ $t('global.action.add') }} <icon-add />
                      </template>
                    </b-form-tags>
                  </b-form-group>
                </b-col>
              </b-row>
            </b-col>
            <b-col lg="3">
              <b-row>
                <b-col lg="12">
                  <p class="col-form-label">
                    {{ $t('pageSslCertificates.modal.privateKey') }}
                  </p>
                  <b-form-group
                    :label="
                      $t('pageSslCertificates.modal.keyPairAlgorithm') + ' *'
                    "
                    label-for="key-pair-algorithm"
                  >
                    <b-form-select
                      id="key-pair-algorithm"
                      v-model="form.keyPairAlgorithm"
                      data-test-id="modalGenerateCsr-select-keyPairAlgorithm"
                      :options="keyPairAlgorithmOptions"
                      :state="getValidationState($v.form.keyPairAlgorithm)"
                      @input="$v.form.keyPairAlgorithm.$touch()"
                    >
                      <template v-slot:first>
                        <b-form-select-option :value="null" disabled>
                          {{ $t('global.form.selectAnOption') }}
                        </b-form-select-option>
                      </template>
                    </b-form-select>
                    <b-form-invalid-feedback role="alert">
                      {{ $t('global.form.fieldRequired') }}
                    </b-form-invalid-feedback>
                  </b-form-group>
                </b-col>
              </b-row>
              <b-row>
                <b-col lg="12">
                  <template v-if="$v.form.keyPairAlgorithm.$model === 'EC'">
                    <b-form-group
                      :label="$t('pageSslCertificates.modal.keyCurveId') + ' *'"
                      label-for="key-curve-id"
                    >
                      <b-form-select
                        id="key-curve-id"
                        v-model="form.keyCurveId"
                        data-test-id="modalGenerateCsr-select-keyCurveId"
                        :options="keyCurveIdOptions"
                        :state="getValidationState($v.form.keyCurveId)"
                        @input="$v.form.keyCurveId.$touch()"
                      >
                        <template v-slot:first>
                          <b-form-select-option :value="null" disabled>
                            {{ $t('global.form.selectAnOption') }}
                          </b-form-select-option>
                        </template>
                      </b-form-select>
                      <b-form-invalid-feedback role="alert">
                        {{ $t('global.form.fieldRequired') }}
                      </b-form-invalid-feedback>
                    </b-form-group>
                  </template>
                  <template v-if="$v.form.keyPairAlgorithm.$model === 'RSA'">
                    <b-form-group
                      :label="
                        $t('pageSslCertificates.modal.keyBitLength') + ' *'
                      "
                      label-for="key-bit-length"
                    >
                      <b-form-select
                        id="key-bit-length"
                        v-model="form.keyBitLength"
                        data-test-id="modalGenerateCsr-select-keyBitLength"
                        :options="keyBitLengthOptions"
                        :state="getValidationState($v.form.keyBitLength)"
                        @input="$v.form.keyBitLength.$touch()"
                      >
                        <template v-slot:first>
                          <b-form-select-option :value="null" disabled>
                            {{ $t('global.form.selectAnOption') }}
                          </b-form-select-option>
                        </template>
                      </b-form-select>
                      <b-form-invalid-feedback role="alert">
                        {{ $t('global.form.fieldRequired') }}
                      </b-form-invalid-feedback>
                    </b-form-group>
                  </template>
                </b-col>
              </b-row>
            </b-col>
          </b-row>
        </b-container>
      </b-form>
      <template v-slot:modal-footer="{ ok, cancel }">
        <b-button variant="secondary" @click="cancel()">
          {{ $t('global.action.cancel') }}
        </b-button>
        <b-button
          form="generate-csr-form"
          type="submit"
          variant="primary"
          data-test-id="modalGenerateCsr-button-ok"
          @click="ok()"
        >
          {{ $t('pageSslCertificates.generateCsr') }}
        </b-button>
      </template>
    </b-modal>
    <b-modal
      id="csr-string"
      no-stacking
      size="lg"
      :title="$t('pageSslCertificates.modal.certificateSigningRequest')"
      @hidden="onHiddenCsrStringModal"
    >
      {{ csrString }}
      <template v-slot:modal-footer>
        <b-btn variant="secondary" @click="copyCsrString">
          <template v-if="csrStringCopied">
            <icon-checkmark />
            {{ $t('global.status.copied') }}
          </template>
          <template v-else>
            {{ $t('global.action.copy') }}
          </template>
        </b-btn>
        <a
          :href="`data:text/json;charset=utf-8,${csrString}`"
          download="certificate.txt"
          class="btn btn-primary"
        >
          {{ $t('global.action.download') }}
        </a>
      </template>
    </b-modal>
  </div>
</template>

<script>
import IconAdd from '@carbon/icons-vue/es/add--alt/20';
import IconCheckmark from '@carbon/icons-vue/es/checkmark/20';

import { required, requiredIf } from 'vuelidate/lib/validators';

import { COUNTRY_LIST } from './CsrCountryCodes';
import { CERTIFICATE_TYPES } from '../../../store/modules/AccessControl/SslCertificatesStore';
import BVToastMixin from '../../../components/Mixins/BVToastMixin';
import VuelidateMixin from '../../../components/Mixins/VuelidateMixin.js';

export default {
  name: 'ModalGenerateCsr',
  components: { IconAdd, IconCheckmark },
  mixins: [BVToastMixin, VuelidateMixin],
  data() {
    return {
      form: {
        certificateType: null,
        country: null,
        state: null,
        city: null,
        companyName: null,
        companyUnit: null,
        commonName: null,
        challengePassword: null,
        contactPerson: null,
        emailAddress: null,
        alternateName: [],
        keyPairAlgorithm: null,
        keyCurveId: null,
        keyBitLength: null
      },
      certificateOptions: CERTIFICATE_TYPES.reduce((arr, cert) => {
        if (cert.type === 'TrustStore Certificate') return arr;
        arr.push({
          text: cert.label,
          value: cert.type
        });
        return arr;
      }, []),
      countryOptions: COUNTRY_LIST.map(country => ({
        text: country.label,
        value: country.code
      })),
      keyPairAlgorithmOptions: ['EC', 'RSA'],
      keyCurveIdOptions: ['prime256v1', 'secp521r1', 'secp384r1'],
      keyBitLengthOptions: [2048],
      csrString: '',
      csrStringCopied: false
    };
  },
  validations: {
    form: {
      certificateType: { required },
      country: { required },
      state: { required },
      city: { required },
      companyName: { required },
      companyUnit: { required },
      commonName: { required },
      challengePassword: {},
      contactPerson: {},
      emailAddress: {},
      alternateName: {},
      keyPairAlgorithm: { required },
      keyCurveId: {
        reuired: requiredIf(function(form) {
          return form.keyPairAlgorithm === 'EC';
        })
      },
      keyBitLength: {
        reuired: requiredIf(function(form) {
          return form.keyPairAlgorithm === 'RSA';
        })
      }
    }
  },
  methods: {
    handleSubmit() {
      this.$v.$touch();
      if (this.$v.$invalid) return;
      this.$store
        .dispatch('sslCertificates/generateCsr', this.form)
        .then(({ data: { CSRString } }) => {
          this.csrString = CSRString;
          this.$bvModal.show('csr-string');
          this.$v.$reset();
        });
    },
    resetForm() {
      for (let key of Object.keys(this.form)) {
        if (key === 'alternateName') {
          this.form[key] = [];
        } else {
          this.form[key] = null;
        }
      }
    },
    onOkGenerateCsrModal(bvModalEvt) {
      // prevent modal close
      bvModalEvt.preventDefault();
      this.handleSubmit();
    },
    onHiddenCsrStringModal() {
      this.csrString = '';
      this.resetForm();
    },
    copyCsrString(bvModalEvt) {
      // prevent modal close
      bvModalEvt.preventDefault();
      navigator.clipboard.writeText(this.csrString).then(() => {
        // Show copied text for 5 seconds
        this.csrStringCopied = true;
        setTimeout(() => {
          this.csrStringCopied = false;
        }, 5000 /*5 seconds*/);
      });
    }
  }
};
</script>
