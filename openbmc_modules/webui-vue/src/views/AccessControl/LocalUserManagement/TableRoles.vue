<template>
  <b-table stacked="sm" small :items="items" :fields="fields">
    <template v-slot:cell(administrator)="data">
      <template v-if="data.value">
        <checkmark20 />
      </template>
    </template>
    <template v-slot:cell(operator)="data">
      <template v-if="data.value">
        <checkmark20 />
      </template>
    </template>
    <template v-slot:cell(readonly)="data">
      <template v-if="data.value">
        <checkmark20 />
      </template>
    </template>
    <template v-slot:cell(noaccess)="data">
      <template v-if="data.value">
        <checkmark20 />
      </template>
    </template>
  </b-table>
</template>

<script>
import Checkmark20 from '@carbon/icons-vue/es/checkmark/20';

export default {
  components: {
    Checkmark20
  },
  data() {
    return {
      items: [
        {
          description: this.$t(
            'pageLocalUserManagement.tableRoles.configureComponentsManagedByThisService'
          ),
          administrator: true,
          operator: false,
          readonly: false,
          noaccess: false
        },
        {
          description: this.$t(
            'pageLocalUserManagement.tableRoles.configureManagerResources'
          ),
          administrator: true,
          operator: false,
          readonly: false,
          noaccess: false
        },
        {
          description: this.$t(
            'pageLocalUserManagement.tableRoles.updatePasswordForCurrentUserAccount'
          ),
          administrator: true,
          operator: false, // TODO Set to true when profile page added
          readonly: false, // TODO Set to true when profile page added
          noaccess: false
        },
        {
          description: this.$t(
            'pageLocalUserManagement.tableRoles.configureUsersAndTheirAccounts'
          ),
          administrator: true,
          operator: false,
          readonly: false,
          noaccess: false
        },
        {
          description: this.$t(
            'pageLocalUserManagement.tableRoles.logInToTheServiceAndReadResources'
          ),
          administrator: true,
          operator: true,
          readonly: true,
          noaccess: false
        },
        {
          description: this.$t(
            'pageLocalUserManagement.tableRoles.ipmiAccessPoint'
          ),
          administrator: true,
          operator: true,
          readonly: true,
          noaccess: false
        },
        {
          description: this.$t(
            'pageLocalUserManagement.tableRoles.redfishAccessPoint'
          ),
          administrator: true,
          operator: true,
          readonly: true,
          noaccess: false
        },
        {
          description: this.$t(
            'pageLocalUserManagement.tableRoles.sshAccessPoint'
          ),
          administrator: true,
          operator: false,
          readonly: false,
          noaccess: false
        },
        {
          description: this.$t(
            'pageLocalUserManagement.tableRoles.webUiAccessPoint'
          ),
          administrator: true,
          operator: true,
          readonly: true,
          noaccess: false
        }
      ],
      fields: [
        { key: 'description', label: '' },
        { key: 'administrator', label: 'Administrator', class: 'text-center' },
        { key: 'operator', label: 'Operator', class: 'text-center' },
        { key: 'readonly', label: 'ReadOnly', class: 'text-center' },
        { key: 'noaccess', label: 'NoAccess', class: 'text-center' }
      ]
    };
  }
};
</script>
