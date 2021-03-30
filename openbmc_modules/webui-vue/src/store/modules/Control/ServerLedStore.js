import api from '../../api';
import i18n from '@/i18n';

const ServerLedStore = {
  namespaced: true,
  state: {
    indicatorValue: 'Off'
  },
  getters: {
    getIndicatorValue: state => state.indicatorValue
  },
  mutations: {
    setIndicatorValue(state, indicatorValue) {
      state.indicatorValue = indicatorValue;
    }
  },
  actions: {
    async getIndicatorValue({ commit }) {
      return await api
        .get('/redfish/v1/Systems/system')
        .then(response => {
          commit('setIndicatorValue', response.data.IndicatorLED);
        })
        .catch(error => console.log(error));
    },
    async saveIndicatorLedValue({ commit }, payload) {
      return await api
        .patch('/redfish/v1/Systems/system', { IndicatorLED: payload })
        .then(() => {
          commit('setIndicatorValue', payload);
          if (payload === 'Lit') {
            return i18n.t('pageServerLed.toast.successServerLedOn');
          } else {
            return i18n.t('pageServerLed.toast.successServerLedOff');
          }
        })
        .catch(error => {
          console.log(error);
          if (payload === 'Lit') {
            throw new Error(i18n.t('pageServerLed.toast.errorServerLedOn'));
          } else {
            throw new Error(i18n.t('pageServerLed.toast.errorServerLedOff'));
          }
        });
    }
  }
};

export default ServerLedStore;
