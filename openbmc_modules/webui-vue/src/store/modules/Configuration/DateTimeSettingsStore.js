import api from '../../api';
import i18n from '@/i18n';

const DateTimeStore = {
  namespaced: true,
  state: {
    ntpServers: [],
    isNtpProtocolEnabled: null
  },
  getters: {
    ntpServers: state => state.ntpServers,
    isNtpProtocolEnabled: state => state.isNtpProtocolEnabled
  },
  mutations: {
    setNtpServers: (state, ntpServers) => (state.ntpServers = ntpServers),
    setIsNtpProtocolEnabled: (state, isNtpProtocolEnabled) =>
      (state.isNtpProtocolEnabled = isNtpProtocolEnabled)
  },
  actions: {
    async getNtpData({ commit }) {
      return await api
        .get('/redfish/v1/Managers/bmc/NetworkProtocol')
        .then(response => {
          const ntpServers = response.data.NTP.NTPServers;
          const isNtpProtocolEnabled = response.data.NTP.ProtocolEnabled;
          commit('setNtpServers', ntpServers);
          commit('setIsNtpProtocolEnabled', isNtpProtocolEnabled);
        })
        .catch(error => {
          console.log(error);
        });
    },
    async updateDateTimeSettings({ state }, dateTimeForm) {
      const ntpData = {
        NTP: {
          ProtocolEnabled: dateTimeForm.ntpProtocolEnabled
        }
      };
      if (dateTimeForm.ntpProtocolEnabled) {
        ntpData.NTP.NTPServers = dateTimeForm.ntpServersArray;
      }
      return await api
        .patch(`/redfish/v1/Managers/bmc/NetworkProtocol`, ntpData)
        .then(async () => {
          if (!dateTimeForm.ntpProtocolEnabled) {
            const dateTimeData = {
              DateTime: dateTimeForm.updatedDateTime
            };
            /**
             * https://github.com/openbmc/phosphor-time-manager/blob/master/README.md#special-note-on-changing-ntp-setting
             * When time mode is initially set to Manual from NTP,
             * NTP service is disabled and the NTP service is
             * stopping but not stopped, setting time will return an error.
             * There are no responses from backend to notify when NTP is stopped.
             * To work around, a timeout is set to allow NTP to fully stop
             * TODO: remove timeout if backend solves
             * https://github.com/openbmc/openbmc/issues/3459
             */
            const timeoutVal = state.isNtpProtocolEnabled ? 20000 : 0;
            return await new Promise((resolve, reject) => {
              setTimeout(() => {
                return api
                  .patch(`/redfish/v1/Managers/bmc`, dateTimeData)
                  .then(() => resolve())
                  .catch(() => reject());
              }, timeoutVal);
            });
          }
        })
        .then(() => {
          return i18n.t(
            'pageDateTimeSettings.toast.successSaveDateTimeSettings'
          );
        })
        .catch(error => {
          console.log(error);
          throw new Error(
            i18n.t('pageDateTimeSettings.toast.errorSaveDateTimeSettings')
          );
        });
    }
  }
};

export default DateTimeStore;
