import api from '../api';

const HOST_STATE = {
  on: 'xyz.openbmc_project.State.Host.HostState.Running',
  off: 'xyz.openbmc_project.State.Host.HostState.Off',
  error: 'xyz.openbmc_project.State.Host.HostState.Quiesced',
  diagnosticMode: 'xyz.openbmc_project.State.Host.HostState.DiagnosticMode'
};

const hostStateMapper = hostState => {
  switch (hostState) {
    case HOST_STATE.on:
    case 'On': // Redfish PowerState
      return 'on';
    case HOST_STATE.off:
    case 'Off': // Redfish PowerState
      return 'off';
    case HOST_STATE.error:
    case 'Quiesced': // Redfish Status
      return 'error';
    case HOST_STATE.diagnosticMode:
    case 'InTest': // Redfish Status
      return 'diagnosticMode';
    default:
      return 'unreachable';
  }
};

const GlobalStore = {
  namespaced: true,
  state: {
    bmcTime: null,
    hostStatus: 'unreachable',
    languagePreference: localStorage.getItem('storedLanguage') || 'en-US',
    isUtcDisplay: localStorage.getItem('storedUtcDisplay')
      ? JSON.parse(localStorage.getItem('storedUtcDisplay'))
      : true,
    username: localStorage.getItem('storedUsername')
  },
  getters: {
    hostStatus: state => state.hostStatus,
    bmcTime: state => state.bmcTime,
    languagePreference: state => state.languagePreference,
    isUtcDisplay: state => state.isUtcDisplay,
    username: state => state.username
  },
  mutations: {
    setBmcTime: (state, bmcTime) => (state.bmcTime = bmcTime),
    setHostStatus: (state, hostState) =>
      (state.hostStatus = hostStateMapper(hostState)),
    setLanguagePreference: (state, language) =>
      (state.languagePreference = language),
    setUsername: (state, username) => (state.username = username),
    setUtcTime: (state, isUtcDisplay) => (state.isUtcDisplay = isUtcDisplay)
  },
  actions: {
    async getBmcTime({ commit }) {
      return await api
        .get('/redfish/v1/Managers/bmc')
        .then(response => {
          const bmcDateTime = response.data.DateTime;
          const date = new Date(bmcDateTime);
          commit('setBmcTime', date);
        })
        .catch(error => console.log(error));
    },
    getHostStatus({ commit }) {
      api
        .get('/redfish/v1/Systems/system')
        .then(({ data: { PowerState, Status: { State } = {} } } = {}) => {
          if (State === 'Quiesced' || State === 'InTest') {
            // OpenBMC's host state interface is mapped to 2 Redfish
            // properties "Status""State" and "PowerState". Look first
            // at State for certain cases.
            commit('setHostStatus', State);
          } else {
            commit('setHostStatus', PowerState);
          }
        })
        .catch(error => console.log(error));
    }
  }
};

export default GlobalStore;
