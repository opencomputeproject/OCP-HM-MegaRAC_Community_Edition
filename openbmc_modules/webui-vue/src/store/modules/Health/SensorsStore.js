import api from '../../api';
import { uniqBy } from 'lodash';

const SensorsStore = {
  namespaced: true,
  state: {
    sensors: []
  },
  getters: {
    sensors: state => state.sensors
  },
  mutations: {
    setSensors: (state, sensors) => {
      state.sensors = uniqBy([...state.sensors, ...sensors], 'name');
    }
  },
  actions: {
    async getAllSensors({ dispatch }) {
      const collection = await dispatch('getChassisCollection');
      if (!collection) return;
      const promises = collection.reduce((acc, id) => {
        acc.push(dispatch('getSensors', id));
        acc.push(dispatch('getThermalSensors', id));
        acc.push(dispatch('getPowerSensors', id));
        return acc;
      }, []);
      return await api.all(promises);
    },
    async getChassisCollection() {
      return await api
        .get('/redfish/v1/Chassis')
        .then(({ data: { Members } }) =>
          Members.map(member => member['@odata.id'])
        )
        .catch(error => console.log(error));
    },
    async getSensors({ commit }, id) {
      const sensors = await api
        .get(`${id}/Sensors`)
        .then(response => response.data.Members)
        .catch(error => console.log(error));
      if (!sensors) return;
      const promises = sensors.map(sensor => {
        return api.get(sensor['@odata.id']).catch(error => {
          console.log(error);
          return error;
        });
      });
      return await api.all(promises).then(
        api.spread((...responses) => {
          const sensorData = responses.map(({ data }) => {
            return {
              name: data.Name,
              status: data.Status.Health,
              currentValue: data.Reading,
              lowerCaution: data.Thresholds.LowerCaution.Reading,
              upperCaution: data.Thresholds.UpperCaution.Reading,
              lowerCritical: data.Thresholds.LowerCritical.Reading,
              upperCritical: data.Thresholds.UpperCritical.Reading,
              units: data.ReadingUnits
            };
          });
          commit('setSensors', sensorData);
        })
      );
    },
    async getThermalSensors({ commit }, id) {
      return await api
        .get(`${id}/Thermal`)
        .then(({ data: { Fans = [], Temperatures = [] } }) => {
          const sensorData = [];
          Fans.forEach(sensor => {
            sensorData.push({
              // TODO: add upper/lower threshold
              name: sensor.Name,
              status: sensor.Status.Health,
              currentValue: sensor.Reading,
              units: sensor.ReadingUnits
            });
          });
          Temperatures.forEach(sensor => {
            sensorData.push({
              name: sensor.Name,
              status: sensor.Status.Health,
              currentValue: sensor.ReadingCelsius,
              lowerCaution: sensor.LowerThresholdNonCritical,
              upperCaution: sensor.UpperThresholdNonCritical,
              lowerCritical: sensor.LowerThresholdCritical,
              upperCritical: sensor.UpperThresholdCritical,
              units: '℃'
            });
          });
          commit('setSensors', sensorData);
        })
        .catch(error => console.log(error));
    },
    async getPowerSensors({ commit }, id) {
      return await api
        .get(`${id}/Power`)
        .then(({ data: { Voltages = [] } }) => {
          const sensorData = Voltages.map(sensor => {
            return {
              name: sensor.Name,
              status: sensor.Status.Health,
              currentValue: sensor.ReadingVolts,
              lowerCaution: sensor.LowerThresholdNonCritical,
              upperCaution: sensor.UpperThresholdNonCritical,
              lowerCritical: sensor.LowerThresholdCritical,
              upperCritical: sensor.UpperThresholdCritical,
              units: 'Volts'
            };
          });
          commit('setSensors', sensorData);
        })
        .catch(error => console.log(error));
    }
  }
};

export default SensorsStore;
