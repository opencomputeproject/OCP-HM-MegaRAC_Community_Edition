<template>
  <b-modal id="upload-certificate" ref="modal" @ok="onOk" @hidden="resetForm">
    <template v-slot:modal-title>
      <template v-if="certificate">
        {{ $t('pageSslCertificates.replaceCertificate') }}
      </template>
      <template v-else>
        {{ $t('pageSslCertificates.addNewCertificate') }}
      </template>
    </template>
    <b-form>
      <!-- Replace Certificate type -->
      <template v-if="certificate !== null">
        <dl class="mb-4">
          <dt>{{ $t('pageSslCertificates.modal.certificateType') }}</dt>
          <dd>{{ certificate.certificate }}</dd>
        </dl>
      </template>

      <!-- Add new Certificate type -->
      <template v-else>
        <b-form-group
          :label="$t('pageSslCertificates.modal.certificateType')"
          label-for="certificate-type"
        >
          <b-form-select
            id="certificate-type"
            v-model="form.certificateType"
            :options="certificateOptions"
            :state="getValidationState($v.form.certificateType)"
            @input="$v.form.certificateType.$touch()"
          >
          </b-form-select>
          <b-form-invalid-feedback role="alert">
            <template v-if="!$v.form.certificateType.required">
              {{ $t('global.form.fieldRequired') }}
            </template>
          </b-form-invalid-feedback>
        </b-form-group>
      </template>

      <b-form-group
        :label="$t('pageSslCertificates.modal.certificateFile')"
        label-for="certificate-file"
      >
        <b-form-file
          id="certificate-file"
          v-model="form.file"
          accept=".pem"
          :browse-text="$t('global.fileUpload.browseText')"
          :drop-placeholder="$t('global.fileUpload.dropPlaceholder')"
          :placeholder="$t('global.fileUpload.placeholder')"
          :state="getValidationState($v.form.file)"
        />
        <b-form-invalid-feedback role="alert">
          <template v-if="!$v.form.file.required">
            {{ $t('global.form.required') }}
          </template>
        </b-form-invalid-feedback>
      </b-form-group>
    </b-form>
    <template v-slot:modal-ok>
      <template v-if="certificate">
        {{ $t('global.action.replace') }}
      </template>
      <template v-else>
        {{ $t('global.action.add') }}
      </template>
    </template>
  </b-modal>
</template>

<script>
import { required, requiredIf } from 'vuelidate/lib/validators';
import VuelidateMixin from '../../../components/Mixins/VuelidateMixin.js';

export default {
  mixins: [VuelidateMixin],
  props: {
    certificate: {
      type: Object,
      default: null,
      validator: prop => {
        if (prop === null) return true;
        return (
          prop.hasOwnProperty('type') && prop.hasOwnProperty('certificate')
        );
      }
    }
  },
  data() {
    return {
      form: {
        certificateType: null,
        file: null
      }
    };
  },
  computed: {
    certificateTypes() {
      return this.$store.getters['sslCertificates/availableUploadTypes'];
    },
    certificateOptions() {
      return this.certificateTypes.map(({ type, label }) => {
        return {
          text: label,
          value: type
        };
      });
    }
  },
  watch: {
    certificateOptions: function(options) {
      if (options.length) {
        this.form.certificateType = options[0].value;
      }
    }
  },
  validations() {
    return {
      form: {
        certificateType: {
          required: requiredIf(function() {
            return !this.certificate;
          })
        },
        file: {
          required
        }
      }
    };
  },
  methods: {
    handleSubmit() {
      this.$v.$touch();
      if (this.$v.$invalid) return;
      this.$emit('ok', {
        addNew: !this.certificate,
        file: this.form.file,
        location: this.certificate ? this.certificate.location : null,
        type: this.certificate
          ? this.certificate.type
          : this.form.certificateType
      });
      this.closeModal();
    },
    closeModal() {
      this.$nextTick(() => {
        this.$refs.modal.hide();
      });
    },
    resetForm() {
      this.form.certificateType = this.certificateOptions.length
        ? this.certificateOptions[0].value
        : null;
      this.form.file = null;
      this.$v.$reset();
    },
    onOk(bvModalEvt) {
      // prevent modal close
      bvModalEvt.preventDefault();
      this.handleSubmit();
    }
  }
};
</script>
