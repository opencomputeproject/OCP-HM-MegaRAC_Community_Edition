import api from '@/store/api';

const ChassisStore = {
  namespaced: true,
  state: {
    chassis: []
  },
  getters: {
    chassis: state => state.chassis
  },
  mutations: {
    setChassisInfo: (state, data) => {
      state.chassis = data.map(chassis => {
        const {
          Id,
          Status = {},
          PartNumber,
          SerialNumber,
          ChassisType,
          Manufacturer,
          PowerState
        } = chassis;

        return {
          id: Id,
          health: Status.Health,
          partNumber: PartNumber,
          serialNumber: SerialNumber,
          chassisType: ChassisType,
          manufacturer: Manufacturer,
          powerState: PowerState,
          statusState: Status.State,
          healthRollup: Status.HealthRollup
        };
      });
    }
  },
  actions: {
    async getChassisInfo({ commit }) {
      return await api
        .get('/redfish/v1/Chassis')
        .then(({ data: { Members = [] } }) =>
          Members.map(member => api.get(member['@odata.id']))
        )
        .then(promises => api.all(promises))
        .then(response => {
          const data = response.map(({ data }) => data);
          commit('setChassisInfo', data);
        })
        .catch(error => console.log(error));
    }
  }
};

export default ChassisStore;
