<template>
  <b-container fluid="xl">
    <page-title />
    <b-row>
      <b-col xl="9" class="text-right">
        <b-button variant="link" @click="initModalSettings">
          <icon-settings />
          {{ $t('pageLocalUserManagement.accountPolicySettings') }}
        </b-button>
        <b-button
          variant="primary"
          data-test-id="localUserManagement-button-addUser"
          @click="initModalUser(null)"
        >
          <icon-add />
          {{ $t('pageLocalUserManagement.addUser') }}
        </b-button>
      </b-col>
    </b-row>
    <b-row>
      <b-col xl="9">
        <table-toolbar
          ref="toolbar"
          :selected-items-count="selectedRows.length"
          :actions="tableToolbarActions"
          @clearSelected="clearSelectedRows($refs.table)"
          @batchAction="onBatchAction"
        />
        <b-table
          ref="table"
          responsive="md"
          selectable
          no-select-on-click
          :fields="fields"
          :items="tableItems"
          @row-selected="onRowSelected($event, tableItems.length)"
        >
          <!-- Checkbox column -->
          <template v-slot:head(checkbox)>
            <b-form-checkbox
              v-model="tableHeaderCheckboxModel"
              data-test-id="localUserManagement-checkbox-tableHeaderCheckbox"
              :indeterminate="tableHeaderCheckboxIndeterminate"
              @change="onChangeHeaderCheckbox($refs.table)"
            />
          </template>
          <template v-slot:cell(checkbox)="row">
            <b-form-checkbox
              v-model="row.rowSelected"
              data-test-id="localUserManagement-checkbox-toggleSelectRow"
              @change="toggleSelectRow($refs.table, row.index)"
            />
          </template>

          <!-- table actions column -->
          <template v-slot:cell(actions)="{ item }">
            <table-row-action
              v-for="(action, index) in item.actions"
              :key="index"
              :value="action.value"
              :enabled="action.enabled"
              :title="action.title"
              @click:tableAction="onTableRowAction($event, item)"
            >
              <template v-slot:icon>
                <icon-edit
                  v-if="action.value === 'edit'"
                  :data-test-id="
                    `localUserManagement-tableRowAction-edit-${index}`
                  "
                />
                <icon-trashcan
                  v-if="action.value === 'delete'"
                  :data-test-id="
                    `localUserManagement-tableRowAction-delete-${index}`
                  "
                />
              </template>
            </table-row-action>
          </template>
        </b-table>
      </b-col>
    </b-row>
    <b-row>
      <b-col xl="8">
        <b-button
          v-b-toggle.collapse-role-table
          data-test-id="localUserManagement-button-viewPrivilegeRoleDescriptions"
          variant="link"
          class="mt-3"
        >
          <icon-chevron />
          {{ $t('pageLocalUserManagement.viewPrivilegeRoleDescriptions') }}
        </b-button>
        <b-collapse id="collapse-role-table" class="mt-3">
          <table-roles />
        </b-collapse>
      </b-col>
    </b-row>
    <!-- Modals -->
    <modal-settings :settings="settings" @ok="saveAccountSettings" />
    <modal-user
      :user="activeUser"
      :password-requirements="passwordRequirements"
      @ok="saveUser"
      @hidden="activeUser = null"
    />
  </b-container>
</template>

<script>
import IconTrashcan from '@carbon/icons-vue/es/trash-can/20';
import IconEdit from '@carbon/icons-vue/es/edit/20';
import IconAdd from '@carbon/icons-vue/es/add--alt/20';
import IconSettings from '@carbon/icons-vue/es/settings/20';
import IconChevron from '@carbon/icons-vue/es/chevron--up/20';

import ModalUser from './ModalUser';
import ModalSettings from './ModalSettings';
import PageTitle from '@/components/Global/PageTitle';
import TableRoles from './TableRoles';
import TableToolbar from '@/components/Global/TableToolbar';
import TableRowAction from '@/components/Global/TableRowAction';

import BVTableSelectableMixin from '@/components/Mixins/BVTableSelectableMixin';
import BVToastMixin from '@/components/Mixins/BVToastMixin';
import LoadingBarMixin from '@/components/Mixins/LoadingBarMixin';

export default {
  name: 'LocalUsers',
  components: {
    IconAdd,
    IconChevron,
    IconEdit,
    IconSettings,
    IconTrashcan,
    ModalSettings,
    ModalUser,
    PageTitle,
    TableRoles,
    TableRowAction,
    TableToolbar
  },
  mixins: [BVTableSelectableMixin, BVToastMixin, LoadingBarMixin],
  data() {
    return {
      activeUser: null,
      fields: [
        {
          key: 'checkbox'
        },
        {
          key: 'username',
          label: this.$t('pageLocalUserManagement.table.username')
        },
        {
          key: 'privilege',
          label: this.$t('pageLocalUserManagement.table.privilege')
        },
        {
          key: 'status',
          label: this.$t('pageLocalUserManagement.table.status')
        },
        {
          key: 'actions',
          label: '',
          tdClass: 'text-right text-nowrap'
        }
      ],
      tableToolbarActions: [
        {
          value: 'delete',
          label: this.$t('global.action.delete')
        },
        {
          value: 'enable',
          label: this.$t('global.action.enable')
        },
        {
          value: 'disable',
          label: this.$t('global.action.disable')
        }
      ]
    };
  },
  computed: {
    allUsers() {
      return this.$store.getters['localUsers/allUsers'];
    },
    tableItems() {
      // transform user data to table data
      return this.allUsers.map(user => {
        return {
          username: user.UserName,
          privilege: user.RoleId,
          status: user.Locked
            ? 'Locked'
            : user.Enabled
            ? 'Enabled'
            : 'Disabled',
          actions: [
            {
              value: 'edit',
              enabled: true,
              title: this.$t('pageLocalUserManagement.editUser')
            },
            {
              value: 'delete',
              enabled: user.UserName === 'root' ? false : true,
              title: this.$tc('pageLocalUserManagement.deleteUser')
            }
          ],
          ...user
        };
      });
    },
    settings() {
      return this.$store.getters['localUsers/accountSettings'];
    },
    passwordRequirements() {
      return this.$store.getters['localUsers/accountPasswordRequirements'];
    }
  },
  created() {
    this.startLoader();
    this.$store.dispatch('localUsers/getUsers').finally(() => this.endLoader());
    this.$store.dispatch('localUsers/getAccountSettings');
    this.$store.dispatch('localUsers/getAccountRoles');
  },
  beforeRouteLeave(to, from, next) {
    this.hideLoader();
    next();
  },
  methods: {
    initModalUser(user) {
      this.activeUser = user;
      this.$bvModal.show('modal-user');
    },
    initModalDelete(user) {
      this.$bvModal
        .msgBoxConfirm(
          this.$t('pageLocalUserManagement.modal.deleteConfirmMessage', {
            user: user.username
          }),
          {
            title: this.$tc('pageLocalUserManagement.deleteUser'),
            okTitle: this.$tc('pageLocalUserManagement.deleteUser')
          }
        )
        .then(deleteConfirmed => {
          if (deleteConfirmed) {
            this.deleteUser(user);
          }
        });
    },
    initModalSettings() {
      this.$bvModal.show('modal-settings');
    },
    saveUser({ isNewUser, userData }) {
      this.startLoader();
      if (isNewUser) {
        this.$store
          .dispatch('localUsers/createUser', userData)
          .then(success => this.successToast(success))
          .catch(({ message }) => this.errorToast(message))
          .finally(() => this.endLoader());
      } else {
        this.$store
          .dispatch('localUsers/updateUser', userData)
          .then(success => this.successToast(success))
          .catch(({ message }) => this.errorToast(message))
          .finally(() => this.endLoader());
      }
    },
    deleteUser({ username }) {
      this.startLoader();
      this.$store
        .dispatch('localUsers/deleteUser', username)
        .then(success => this.successToast(success))
        .catch(({ message }) => this.errorToast(message))
        .finally(() => this.endLoader());
    },
    onBatchAction(action) {
      switch (action) {
        case 'delete':
          this.$bvModal
            .msgBoxConfirm(
              this.$tc(
                'pageLocalUserManagement.modal.batchDeleteConfirmMessage',
                this.selectedRows.length
              ),
              {
                title: this.$tc(
                  'pageLocalUserManagement.deleteUser',
                  this.selectedRows.length
                ),
                okTitle: this.$tc(
                  'pageLocalUserManagement.deleteUser',
                  this.selectedRows.length
                )
              }
            )
            .then(deleteConfirmed => {
              if (deleteConfirmed) {
                this.startLoader();
                this.$store
                  .dispatch('localUsers/deleteUsers', this.selectedRows)
                  .then(messages => {
                    messages.forEach(({ type, message }) => {
                      if (type === 'success') this.successToast(message);
                      if (type === 'error') this.errorToast(message);
                    });
                  })
                  .finally(() => this.endLoader());
              }
            });
          break;
        case 'enable':
          this.startLoader();
          this.$store
            .dispatch('localUsers/enableUsers', this.selectedRows)
            .then(messages => {
              messages.forEach(({ type, message }) => {
                if (type === 'success') this.successToast(message);
                if (type === 'error') this.errorToast(message);
              });
            })
            .finally(() => this.endLoader());
          break;
        case 'disable':
          this.startLoader();
          this.$store
            .dispatch('localUsers/disableUsers', this.selectedRows)
            .then(messages => {
              messages.forEach(({ type, message }) => {
                if (type === 'success') this.successToast(message);
                if (type === 'error') this.errorToast(message);
              });
            })
            .finally(() => this.endLoader());
          break;
      }
    },
    onTableRowAction(action, row) {
      switch (action) {
        case 'edit':
          this.initModalUser(row);
          break;
        case 'delete':
          this.initModalDelete(row);
          break;
        default:
          break;
      }
    },
    saveAccountSettings(settings) {
      this.startLoader();
      this.$store
        .dispatch('localUsers/saveAccountSettings', settings)
        .then(message => this.successToast(message))
        .catch(({ message }) => this.errorToast(message))
        .finally(() => this.endLoader());
    }
  }
};
</script>

<style lang="scss" scoped>
.btn.collapsed {
  svg {
    transform: rotate(180deg);
  }
}
</style>
