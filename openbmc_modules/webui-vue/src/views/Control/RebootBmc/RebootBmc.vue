<template>
  <b-container fluid="xl">
    <page-title />
    <b-row>
      <b-col md="8" lg="8" xl="6">
        <page-section>
          {{ $t('pageRebootBmc.rebootInformation') }}
          <b-button
            variant="primary"
            class="d-block mt-5"
            data-test-id="rebootBmc-button-reboot"
            @click="onClick"
          >
            {{ $t('pageRebootBmc.rebootBmc') }}
          </b-button>
        </page-section>
      </b-col>
    </b-row>
  </b-container>
</template>

<script>
import PageTitle from '../../../components/Global/PageTitle';
import PageSection from '../../../components/Global/PageSection';
import BVToastMixin from '../../../components/Mixins/BVToastMixin';

export default {
  name: 'RebootBmc',
  components: { PageTitle, PageSection },
  mixins: [BVToastMixin],
  methods: {
    onClick() {
      this.$bvModal
        .msgBoxConfirm(this.$t('pageRebootBmc.modal.confirmMessage'), {
          title: this.$t('pageRebootBmc.modal.confirmTitle'),
          okTitle: this.$t('global.action.confirm')
        })
        .then(confirmed => {
          if (confirmed) this.rebootBmc();
        });
    },
    rebootBmc() {
      this.$store
        .dispatch('controls/rebootBmc')
        .then(message => this.successToast(message))
        .catch(({ message }) => this.errorToast(message));
    }
  }
};
</script>

<style lang="scss" scoped></style>
