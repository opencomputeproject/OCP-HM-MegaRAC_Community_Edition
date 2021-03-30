import api, { getResponseCount } from '@/store/api';
import i18n from '@/i18n';

const getHealthStatus = (events, loadedEvents) => {
  let status = loadedEvents ? 'OK' : '';
  for (const event of events) {
    if (event.severity === 'Warning') {
      status = 'Warning';
    }
    if (event.severity === 'Critical') {
      status = 'Critical';
      break;
    }
  }
  return status;
};

// TODO: High priority events should also check if Log
// is resolved when the property is available in Redfish
const getHighPriorityEvents = events =>
  events.filter(({ severity }) => severity === 'Critical');

const EventLogStore = {
  namespaced: true,
  state: {
    allEvents: [],
    loadedEvents: false
  },
  getters: {
    allEvents: state => state.allEvents,
    highPriorityEvents: state => getHighPriorityEvents(state.allEvents),
    healthStatus: state => getHealthStatus(state.allEvents, state.loadedEvents)
  },
  mutations: {
    setAllEvents: (state, allEvents) => (
      (state.allEvents = allEvents), (state.loadedEvents = true)
    )
  },
  actions: {
    async getEventLogData({ commit }) {
      return await api
        .get('/redfish/v1/Systems/system/LogServices/EventLog/Entries')
        .then(({ data: { Members = [] } = {} }) => {
          const eventLogs = Members.map(log => {
            const { Id, Severity, Created, EntryType, Message } = log;
            return {
              id: Id,
              severity: Severity,
              date: new Date(Created),
              type: EntryType,
              description: Message,
              uri: log['@odata.id']
            };
          });
          commit('setAllEvents', eventLogs);
        })
        .catch(error => {
          console.log('Event Log Data:', error);
        });
    },
    async deleteEventLogs({ dispatch }, uris = []) {
      const promises = uris.map(uri =>
        api.delete(uri).catch(error => {
          console.log(error);
          return error;
        })
      );
      return await api
        .all(promises)
        .then(response => {
          dispatch('getEventLogData');
          return response;
        })
        .then(
          api.spread((...responses) => {
            const { successCount, errorCount } = getResponseCount(responses);
            const toastMessages = [];

            if (successCount) {
              const message = i18n.tc(
                'pageEventLogs.toast.successDelete',
                successCount
              );
              toastMessages.push({ type: 'success', message });
            }

            if (errorCount) {
              const message = i18n.tc(
                'pageEventLogs.toast.errorDelete',
                errorCount
              );
              toastMessages.push({ type: 'error', message });
            }

            return toastMessages;
          })
        );
    }
  }
};

export default EventLogStore;
