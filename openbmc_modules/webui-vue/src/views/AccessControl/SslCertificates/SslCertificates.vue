<template>
  <b-container fluid="xl">
    <page-title />
    <b-row>
      <b-col xl="11">
        <!-- Expired certificates banner -->
        <alert :show="expiredCertificateTypes.length > 0" variant="danger">
          <template v-if="expiredCertificateTypes.length > 1">
            {{ $t('pageSslCertificates.alert.certificatesExpiredMessage') }}
          </template>
          <template v-else>
            {{
              $t('pageSslCertificates.alert.certificateExpiredMessage', {
                certificate: expiredCertificateTypes[0]
              })
            }}
          </template>
        </alert>
        <!-- Expiring certificates banner -->
        <alert :show="expiringCertificateTypes.length > 0" variant="warning">
          <template v-if="expiringCertificateTypes.length > 1">
            {{ $t('pageSslCertificates.alert.certificatesExpiringMessage') }}
          </template>
          <template v-else>
            {{
              $t('pageSslCertificates.alert.certificateExpiringMessage', {
                certificate: expiringCertificateTypes[0]
              })
            }}
          </template>
        </alert>
      </b-col>
    </b-row>
    <b-row>
      <b-col xl="11" class="text-right">
        <b-button
          v-b-modal.generate-csr
          data-test-id="sslCertificates-button-generateCsr"
          variant="link"
        >
          <icon-add />
          {{ $t('pageSslCertificates.generateCsr') }}
        </b-button>
        <b-button
          variant="primary"
          :disabled="certificatesForUpload.length === 0"
          @click="initModalUploadCertificate(null)"
        >
          <icon-add />
          {{ $t('pageSslCertificates.addNewCertificate') }}
        </b-button>
      </b-col>
    </b-row>
    <b-row>
      <b-col xl="11">
        <b-table responsive="md" :fields="fields" :items="tableItems">
          <template v-slot:cell(validFrom)="{ value }">
            {{ value | formatDate }}
          </template>

          <template v-slot:cell(validUntil)="{ value }">
            <status-icon
              v-if="getDaysUntilExpired(value) < 31"
              :status="getIconStatus(value)"
            />
            {{ value | formatDate }}
          </template>

          <template v-slot:cell(actions)="{ value, item }">
            <table-row-action
              v-for="(action, index) in value"
              :key="index"
              :value="action.value"
              :title="action.title"
              :enabled="action.enabled"
              @click:tableAction="onTableRowAction($event, item)"
            >
              <template v-slot:icon>
                <icon-replace v-if="action.value === 'replace'" />
                <icon-trashcan v-if="action.value === 'delete'" />
              </template>
            </table-row-action>
          </template>
        </b-table>
      </b-col>
    </b-row>

    <!-- Modals -->
    <modal-upload-certificate :certificate="modalCertificate" @ok="onModalOk" />
    <modal-generate-csr />
  </b-container>
</template>

<script>
import IconAdd from '@carbon/icons-vue/es/add--alt/20';
import IconReplace from '@carbon/icons-vue/es/renew/20';
import IconTrashcan from '@carbon/icons-vue/es/trash-can/20';

import ModalGenerateCsr from './ModalGenerateCsr';
import ModalUploadCertificate from './ModalUploadCertificate';
import PageTitle from '@/components/Global/PageTitle';
import TableRowAction from '@/components/Global/TableRowAction';
import StatusIcon from '@/components/Global/StatusIcon';
import Alert from '@/components/Global/Alert';

import BVToastMixin from '@/components/Mixins/BVToastMixin';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';

export default {
  name: 'SslCertificates',
  components: {
    Alert,
    IconAdd,
    IconReplace,
    IconTrashcan,
    ModalGenerateCsr,
    ModalUploadCertificate,
    PageTitle,
    StatusIcon,
    TableRowAction
  },
  mixins: [BVToastMixin, LoadingBarMixin],
  data() {
    return {
      modalCertificate: null,
      fields: [
        {
          key: 'certificate',
          label: this.$t('pageSslCertificates.table.certificate')
        },
        {
          key: 'issuedBy',
          label: this.$t('pageSslCertificates.table.issuedBy')
        },
        {
          key: 'issuedTo',
          label: this.$t('pageSslCertificates.table.issuedTo')
        },
        {
          key: 'validFrom',
          label: this.$t('pageSslCertificates.table.validFrom')
        },
        {
          key: 'validUntil',
          label: this.$t('pageSslCertificates.table.validUntil')
        },
        {
          key: 'actions',
          label: '',
          tdClass: 'text-right text-nowrap'
        }
      ]
    };
  },
  computed: {
    certificates() {
      return this.$store.getters['sslCertificates/allCertificates'];
    },
    tableItems() {
      return this.certificates.map(certificate => {
        return {
          ...certificate,
          actions: [
            {
              value: 'replace',
              title: this.$t('pageSslCertificates.replaceCertificate')
            },
            {
              value: 'delete',
              title: this.$t('pageSslCertificates.deleteCertificate'),
              enabled:
                certificate.type === 'TrustStore Certificate' ? true : false
            }
          ]
        };
      });
    },
    certificatesForUpload() {
      return this.$store.getters['sslCertificates/availableUploadTypes'];
    },
    bmcTime() {
      return this.$store.getters['global/bmcTime'];
    },
    expiredCertificateTypes() {
      return this.certificates.reduce((acc, val) => {
        const daysUntilExpired = this.getDaysUntilExpired(val.validUntil);
        if (daysUntilExpired < 1) {
          acc.push(val.certificate);
        }
        return acc;
      }, []);
    },
    expiringCertificateTypes() {
      return this.certificates.reduce((acc, val) => {
        const daysUntilExpired = this.getDaysUntilExpired(val.validUntil);
        if (daysUntilExpired < 31 && daysUntilExpired > 0) {
          acc.push(val.certificate);
        }
        return acc;
      }, []);
    }
  },
  async created() {
    this.startLoader();
    await this.$store.dispatch('global/getBmcTime');
    this.$store
      .dispatch('sslCertificates/getCertificates')
      .finally(() => this.endLoader());
  },
  beforeRouteLeave(to, from, next) {
    this.hideLoader();
    next();
  },
  methods: {
    onTableRowAction(event, rowItem) {
      switch (event) {
        case 'replace':
          this.initModalUploadCertificate(rowItem);
          break;
        case 'delete':
          this.initModalDeleteCertificate(rowItem);
          break;
        default:
          break;
      }
    },
    initModalUploadCertificate(certificate = null) {
      this.modalCertificate = certificate;
      this.$bvModal.show('upload-certificate');
    },
    initModalDeleteCertificate(certificate) {
      this.$bvModal
        .msgBoxConfirm(
          this.$t('pageSslCertificates.modal.deleteConfirmMessage', {
            issuedBy: certificate.issuedBy,
            certificate: certificate.certificate
          }),
          {
            title: this.$t('pageSslCertificates.deleteCertificate'),
            okTitle: this.$t('global.action.delete')
          }
        )
        .then(deleteConfirmed => {
          if (deleteConfirmed) this.deleteCertificate(certificate);
        });
    },
    onModalOk({ addNew, file, type, location }) {
      if (addNew) {
        // Upload a new certificate
        this.addNewCertificate(file, type);
      } else {
        // Replace an existing certificate
        this.replaceCertificate(file, type, location);
      }
    },
    addNewCertificate(file, type) {
      this.startLoader();
      this.$store
        .dispatch('sslCertificates/addNewCertificate', { file, type })
        .then(success => this.successToast(success))
        .catch(({ message }) => this.errorToast(message))
        .finally(() => this.endLoader());
    },
    replaceCertificate(file, type, location) {
      this.startLoader();
      const reader = new FileReader();
      reader.readAsBinaryString(file);
      reader.onloadend = event => {
        const certificateString = event.target.result;
        this.$store
          .dispatch('sslCertificates/replaceCertificate', {
            certificateString,
            type,
            location
          })
          .then(success => this.successToast(success))
          .catch(({ message }) => this.errorToast(message))
          .finally(() => this.endLoader());
      };
    },
    deleteCertificate({ type, location }) {
      this.startLoader();
      this.$store
        .dispatch('sslCertificates/deleteCertificate', {
          type,
          location
        })
        .then(success => this.successToast(success))
        .catch(({ message }) => this.errorToast(message))
        .finally(() => this.endLoader());
    },
    getDaysUntilExpired(date) {
      if (this.bmcTime) {
        const validUntilMs = date.getTime();
        const currentBmcTimeMs = this.bmcTime.getTime();
        const oneDayInMs = 24 * 60 * 60 * 1000;
        return Math.round((validUntilMs - currentBmcTimeMs) / oneDayInMs);
      }
      return new Date();
    },
    getIconStatus(date) {
      const daysUntilExpired = this.getDaysUntilExpired(date);
      if (daysUntilExpired < 1) {
        return 'danger';
      } else if (daysUntilExpired < 31) {
        return 'warning';
      }
    }
  }
};
</script>
