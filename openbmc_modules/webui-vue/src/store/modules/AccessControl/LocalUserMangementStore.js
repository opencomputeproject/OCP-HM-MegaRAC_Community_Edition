import api, { getResponseCount } from '@/store/api';
import i18n from '@/i18n';

const LocalUserManagementStore = {
  namespaced: true,
  state: {
    allUsers: [],
    accountRoles: [],
    accountLockoutDuration: null,
    accountLockoutThreshold: null,
    accountMinPasswordLength: null,
    accountMaxPasswordLength: null
  },
  getters: {
    allUsers(state) {
      return state.allUsers;
    },
    accountRoles(state) {
      return state.accountRoles;
    },
    accountSettings(state) {
      return {
        lockoutDuration: state.accountLockoutDuration,
        lockoutThreshold: state.accountLockoutThreshold
      };
    },
    accountPasswordRequirements(state) {
      return {
        minLength: state.accountMinPasswordLength,
        maxLength: state.accountMaxPasswordLength
      };
    }
  },
  mutations: {
    setUsers(state, allUsers) {
      state.allUsers = allUsers;
    },
    setAccountRoles(state, accountRoles) {
      state.accountRoles = accountRoles;
    },
    setLockoutDuration(state, lockoutDuration) {
      state.accountLockoutDuration = lockoutDuration;
    },
    setLockoutThreshold(state, lockoutThreshold) {
      state.accountLockoutThreshold = lockoutThreshold;
    },
    setAccountMinPasswordLength(state, minPasswordLength) {
      state.accountMinPasswordLength = minPasswordLength;
    },
    setAccountMaxPasswordLength(state, maxPasswordLength) {
      state.accountMaxPasswordLength = maxPasswordLength;
    }
  },
  actions: {
    async getUsers({ commit }) {
      return await api
        .get('/redfish/v1/AccountService/Accounts')
        .then(response => response.data.Members.map(user => user['@odata.id']))
        .then(userIds => api.all(userIds.map(user => api.get(user))))
        .then(users => {
          const userData = users.map(user => user.data);
          commit('setUsers', userData);
        })
        .catch(error => {
          console.log(error);
          const message = i18n.t(
            'pageLocalUserManagement.toast.errorLoadUsers'
          );
          throw new Error(message);
        });
    },
    getAccountSettings({ commit }) {
      api
        .get('/redfish/v1/AccountService')
        .then(({ data }) => {
          commit('setLockoutDuration', data.AccountLockoutDuration);
          commit('setLockoutThreshold', data.AccountLockoutThreshold);
          commit('setAccountMinPasswordLength', data.MinPasswordLength);
          commit('setAccountMaxPasswordLength', data.MaxPasswordLength);
        })
        .catch(error => {
          console.log(error);
          const message = i18n.t(
            'pageLocalUserManagement.toast.errorLoadAccountSettings'
          );
          throw new Error(message);
        });
    },
    getAccountRoles({ commit }) {
      api
        .get('/redfish/v1/AccountService/Roles')
        .then(({ data: { Members = [] } = {} }) => {
          const roles = Members.map(role => {
            return role['@odata.id'].split('/').pop();
          });
          commit('setAccountRoles', roles);
        })
        .catch(error => console.log(error));
    },
    async createUser({ dispatch }, { username, password, privilege, status }) {
      const data = {
        UserName: username,
        Password: password,
        RoleId: privilege,
        Enabled: status
      };
      return await api
        .post('/redfish/v1/AccountService/Accounts', data)
        .then(() => dispatch('getUsers'))
        .then(() =>
          i18n.t('pageLocalUserManagement.toast.successCreateUser', {
            username
          })
        )
        .catch(error => {
          console.log(error);
          const message = i18n.t(
            'pageLocalUserManagement.toast.errorCreateUser',
            { username }
          );
          throw new Error(message);
        });
    },
    async updateUser(
      { dispatch },
      { originalUsername, username, password, privilege, status, locked }
    ) {
      const data = {};
      if (username) data.UserName = username;
      if (password) data.Password = password;
      if (privilege) data.RoleId = privilege;
      if (status !== undefined) data.Enabled = status;
      if (locked !== undefined) data.Locked = locked;
      return await api
        .patch(`/redfish/v1/AccountService/Accounts/${originalUsername}`, data)
        .then(() => dispatch('getUsers'))
        .then(() =>
          i18n.t('pageLocalUserManagement.toast.successUpdateUser', {
            username: originalUsername
          })
        )
        .catch(error => {
          console.log(error);
          const message = i18n.t(
            'pageLocalUserManagement.toast.errorUpdateUser',
            { username: originalUsername }
          );
          throw new Error(message);
        });
    },
    async deleteUser({ dispatch }, username) {
      return await api
        .delete(`/redfish/v1/AccountService/Accounts/${username}`)
        .then(() => dispatch('getUsers'))
        .then(() =>
          i18n.t('pageLocalUserManagement.toast.successDeleteUser', {
            username
          })
        )
        .catch(error => {
          console.log(error);
          const message = i18n.t(
            'pageLocalUserManagement.toast.errorDeleteUser',
            { username }
          );
          throw new Error(message);
        });
    },
    async deleteUsers({ dispatch }, users) {
      const promises = users.map(({ username }) => {
        return api
          .delete(`/redfish/v1/AccountService/Accounts/${username}`)
          .catch(error => {
            console.log(error);
            return error;
          });
      });
      return await api
        .all(promises)
        .then(response => {
          dispatch('getUsers');
          return response;
        })
        .then(
          api.spread((...responses) => {
            const { successCount, errorCount } = getResponseCount(responses);
            let toastMessages = [];

            if (successCount) {
              const message = i18n.tc(
                'pageLocalUserManagement.toast.successBatchDelete',
                successCount
              );
              toastMessages.push({ type: 'success', message });
            }

            if (errorCount) {
              const message = i18n.tc(
                'pageLocalUserManagement.toast.errorBatchDelete',
                errorCount
              );
              toastMessages.push({ type: 'error', message });
            }

            return toastMessages;
          })
        );
    },
    async enableUsers({ dispatch }, users) {
      const data = {
        Enabled: true
      };
      const promises = users.map(({ username }) => {
        return api
          .patch(`/redfish/v1/AccountService/Accounts/${username}`, data)
          .catch(error => {
            console.log(error);
            return error;
          });
      });
      return await api
        .all(promises)
        .then(response => {
          dispatch('getUsers');
          return response;
        })
        .then(
          api.spread((...responses) => {
            const { successCount, errorCount } = getResponseCount(responses);
            let toastMessages = [];

            if (successCount) {
              const message = i18n.tc(
                'pageLocalUserManagement.toast.successBatchEnable',
                successCount
              );
              toastMessages.push({ type: 'success', message });
            }

            if (errorCount) {
              const message = i18n.tc(
                'pageLocalUserManagement.toast.errorBatchEnable',
                errorCount
              );
              toastMessages.push({ type: 'error', message });
            }

            return toastMessages;
          })
        );
    },
    async disableUsers({ dispatch }, users) {
      const data = {
        Enabled: false
      };
      const promises = users.map(({ username }) => {
        return api
          .patch(`/redfish/v1/AccountService/Accounts/${username}`, data)
          .catch(error => {
            console.log(error);
            return error;
          });
      });
      return await api
        .all(promises)
        .then(response => {
          dispatch('getUsers');
          return response;
        })
        .then(
          api.spread((...responses) => {
            const { successCount, errorCount } = getResponseCount(responses);
            let toastMessages = [];

            if (successCount) {
              const message = i18n.tc(
                'pageLocalUserManagement.toast.successBatchDisable',
                successCount
              );
              toastMessages.push({ type: 'success', message });
            }

            if (errorCount) {
              const message = i18n.tc(
                'pageLocalUserManagement.toast.errorBatchDisable',
                errorCount
              );
              toastMessages.push({ type: 'error', message });
            }

            return toastMessages;
          })
        );
    },
    async saveAccountSettings(
      { dispatch },
      { lockoutThreshold, lockoutDuration }
    ) {
      const data = {};
      if (lockoutThreshold !== undefined) {
        data.AccountLockoutThreshold = lockoutThreshold;
      }
      if (lockoutDuration !== undefined) {
        data.AccountLockoutDuration = lockoutDuration;
      }

      return await api
        .patch('/redfish/v1/AccountService', data)
        //GET new settings to update view
        .then(() => dispatch('getAccountSettings'))
        .then(() => i18n.t('pageLocalUserManagement.toast.successSaveSettings'))
        .catch(error => {
          console.log(error);
          const message = i18n.t(
            'pageLocalUserManagement.toast.errorSaveSettings'
          );
          throw new Error(message);
        });
    }
  }
};

export default LocalUserManagementStore;
