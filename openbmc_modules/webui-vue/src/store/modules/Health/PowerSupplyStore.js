import api from '@/store/api';

const PowerSupplyStore = {
  namespaced: true,
  state: {
    powerSupplies: []
  },
  getters: {
    powerSupplies: state => state.powerSupplies
  },
  mutations: {
    setPowerSupply: (state, data) => {
      state.powerSupplies = data.map(powerSupply => {
        const {
          EfficiencyPercent,
          FirmwareVersion,
          IndicatorLED,
          MemberId,
          Model,
          PartNumber,
          PowerInputWatts,
          SerialNumber,
          Status
        } = powerSupply;
        return {
          id: MemberId,
          health: Status.Health,
          partNumber: PartNumber,
          serialNumber: SerialNumber,
          efficiencyPercent: EfficiencyPercent,
          firmwareVersion: FirmwareVersion,
          indicatorLed: IndicatorLED,
          model: Model,
          powerInputWatts: PowerInputWatts,
          statusState: Status.State
        };
      });
    }
  },
  actions: {
    async getPowerSupply({ commit }) {
      return await api
        .get('/redfish/v1/Chassis/chassis/Power')
        .then(({ data: { PowerSupplies } }) =>
          commit('setPowerSupply', PowerSupplies)
        )
        .catch(error => console.log(error));
    }
  }
};

export default PowerSupplyStore;
