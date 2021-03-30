import api from '../../api';

const FirmwareStore = {
  namespaced: true,
  state: {
    bmcFirmwareVersion: '--'
  },
  getters: {
    bmcFirmwareVersion: state => state.bmcFirmwareVersion
  },
  mutations: {
    setBmcFirmwareVersion: (state, bmcFirmwareVersion) =>
      (state.bmcFirmwareVersion = bmcFirmwareVersion)
  },
  actions: {
    async getBmcFirmware({ commit }) {
      return await api
        .get('/redfish/v1/Managers/bmc')
        .then(response => {
          const bmcFirmwareVersion = response.data.FirmwareVersion;
          commit('setBmcFirmwareVersion', bmcFirmwareVersion);
        })
        .catch(error => {
          console.log(error);
        });
    }
  }
};

export default FirmwareStore;
