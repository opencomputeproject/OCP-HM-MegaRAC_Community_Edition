import api from '../../api';
import i18n from '../../../i18n';

/**
 * Watch for hostStatus changes in GlobalStore module
 * to set isOperationInProgress state
 * Stop watching status changes and resolve Promise when
 * hostStatus value matches passed argument or after 5 minutes
 * @param {string} hostStatus
 * @returns {Promise}
 */
const checkForHostStatus = function(hostStatus) {
  return new Promise(resolve => {
    const timer = setTimeout(() => {
      resolve();
      unwatch();
    }, 300000 /*5mins*/);
    const unwatch = this.watch(
      state => state.global.hostStatus,
      value => {
        if (value === hostStatus) {
          resolve();
          unwatch();
          clearTimeout(timer);
        }
      }
    );
  });
};

const ControlStore = {
  namespaced: true,
  state: {
    isOperationInProgress: false
  },
  getters: {
    isOperationInProgress: state => state.isOperationInProgress
  },
  mutations: {
    setOperationInProgress: (state, inProgress) =>
      (state.isOperationInProgress = inProgress)
  },
  actions: {
    async rebootBmc() {
      const data = { ResetType: 'GracefulRestart' };
      return await api
        .post('/redfish/v1/Managers/bmc/Actions/Manager.Reset', data)
        .then(() => i18n.t('pageRebootBmc.toast.successRebootStart'))
        .catch(error => {
          console.log(error);
          throw new Error(i18n.t('pageRebootBmc.toast.errorRebootStart'));
        });
    },
    async hostPowerOn({ dispatch, commit }) {
      const data = { ResetType: 'On' };
      dispatch('hostPowerChange', data);
      await checkForHostStatus.bind(this, 'on')();
      commit('setOperationInProgress', false);
    },
    async hostSoftReboot({ dispatch, commit }) {
      const data = { ResetType: 'GracefulRestart' };
      dispatch('hostPowerChange', data);
      await checkForHostStatus.bind(this, 'on')();
      commit('setOperationInProgress', false);
    },
    async hostHardReboot({ dispatch, commit }) {
      // TODO: Update when ForceWarmReboot property
      // available
      dispatch('hostPowerChange', { ResetType: 'ForceOff' });
      await checkForHostStatus.bind(this, 'off')();
      dispatch('hostPowerChange', { ResetType: 'On' });
      await checkForHostStatus.bind(this, 'on')();
      commit('setOperationInProgress', false);
    },
    async hostSoftPowerOff({ dispatch, commit }) {
      const data = { ResetType: 'GracefulShutdown' };
      dispatch('hostPowerChange', data);
      await checkForHostStatus.bind(this, 'off')();
      commit('setOperationInProgress', false);
    },
    async hostHardPowerOff({ dispatch, commit }) {
      const data = { ResetType: 'ForceOff' };
      dispatch('hostPowerChange', data);
      await checkForHostStatus.bind(this, 'off')();
      commit('setOperationInProgress', false);
    },
    hostPowerChange({ commit }, data) {
      commit('setOperationInProgress', true);
      api
        .post('/redfish/v1/Systems/system/Actions/ComputerSystem.Reset', data)
        .catch(error => {
          console.log(error);
          commit('setOperationInProgress', false);
        });
    }
  }
};

export default ControlStore;
