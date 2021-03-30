import api from '../../api';
import i18n from '@/i18n';
import { find, remove } from 'lodash';

const NetworkSettingsStore = {
  namespaced: true,
  state: {
    defaultGateway: '',
    ethernetData: [],
    interfaceOptions: []
  },
  getters: {
    defaultGateway: state => state.defaultGateway,
    ethernetData: state => state.ethernetData,
    interfaceOptions: state => state.interfaceOptions
  },
  mutations: {
    setDefaultGateway: (state, defaultGateway) =>
      (state.defaultGateway = defaultGateway),
    setEthernetData: (state, ethernetData) =>
      (state.ethernetData = ethernetData),
    setInterfaceOptions: (state, interfaceOptions) =>
      (state.interfaceOptions = interfaceOptions)
  },
  actions: {
    async getEthernetData({ commit }) {
      return await api
        .get('/redfish/v1/Managers/bmc/EthernetInterfaces')
        .then(response =>
          response.data.Members.map(
            ethernetInterface => ethernetInterface['@odata.id']
          )
        )
        .then(ethernetInterfaceIds =>
          api.all(
            ethernetInterfaceIds.map(ethernetInterface =>
              api.get(ethernetInterface)
            )
          )
        )
        .then(ethernetInterfaces => {
          const ethernetData = ethernetInterfaces.map(
            ethernetInterface => ethernetInterface.data
          );
          const interfaceOptions = ethernetInterfaces.map(
            ethernetName => ethernetName.data.Id
          );
          const addresses = ethernetData[0].IPv4StaticAddresses;

          // Default gateway manually set to first gateway saved on the first interface. Default gateway property is WIP on backend
          const defaultGateway = addresses.map(ipv4 => {
            return ipv4.Gateway;
          });

          commit('setDefaultGateway', defaultGateway[0]);
          commit('setEthernetData', ethernetData);
          commit('setInterfaceOptions', interfaceOptions);
        })
        .catch(error => {
          console.log('Network Data:', error);
        });
    },

    async updateInterfaceSettings({ dispatch, state }, networkSettingsForm) {
      const updatedAddresses = networkSettingsForm.staticIpv4;
      const originalAddresses =
        state.ethernetData[networkSettingsForm.selectedInterfaceIndex]
          .IPv4StaticAddresses;

      const addressArray = originalAddresses.map(item => {
        const address = item.Address;
        if (find(updatedAddresses, { Address: address })) {
          remove(updatedAddresses, item => {
            return item.Address === address;
          });
          return {};
        } else {
          return null;
        }
      });

      const data = {
        HostName: networkSettingsForm.hostname,
        MACAddress: networkSettingsForm.macAddress
      };

      // If DHCP disabled, update static DNS or static ipv4
      if (!networkSettingsForm.isDhcpEnabled) {
        data.IPv4StaticAddresses = [...addressArray, ...updatedAddresses];
        data.StaticNameServers = networkSettingsForm.staticNameServers;
      }

      return await api
        .patch(
          `/redfish/v1/Managers/bmc/EthernetInterfaces/${networkSettingsForm.interfaceId}`,
          data
        )
        .then(() => dispatch('getEthernetData'))
        .then(() => {
          return i18n.t('pageNetworkSettings.toast.successSaveNetworkSettings');
        })
        .catch(error => {
          console.log(error);
          throw new Error(
            i18n.t('pageNetworkSettings.toast.errorSaveNetworkSettings')
          );
        });
    }
  }
};

export default NetworkSettingsStore;
