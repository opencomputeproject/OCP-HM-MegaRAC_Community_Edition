import { debounce } from 'lodash';

/**
 * WebSocketPlugin will allow us to get new data from the server
 * without having to poll for changes on the frontend.
 *
 * This plugin is subscribed to host state property and logging
 * changes, indicated in the app header Health and Power status.
 *
 * https://github.com/openbmc/docs/blob/b41aff0fabe137cdb0cfff584b5fe4a41c0c8e77/rest-api.md#event-subscription-protocol
 */
const WebSocketPlugin = store => {
  let ws;
  const data = {
    paths: ['/xyz/openbmc_project/state/host0', '/xyz/openbmc_project/logging'],
    interfaces: [
      'xyz.openbmc_project.State.Host',
      'xyz.openbmc_project.Logging.Entry'
    ]
  };

  const initWebSocket = () => {
    const socketDisabled =
      process.env.VUE_APP_SUBSCRIBE_SOCKET_DISABLED === 'true' ? true : false;
    if (socketDisabled) return;
    const token = store.getters['authentication/token'];
    ws = new WebSocket(`wss://${window.location.host}/subscribe`, [token]);
    ws.onopen = () => {
      ws.send(JSON.stringify(data));
    };
    ws.on('error', function(err) {
      console.error('error!');
      console.error(err.code);
    });
    ws.onerror = event => {
      console.error(event);
    };
    ws.onmessage = debounce(event => {
      const data = JSON.parse(event.data);
      const eventInterface = data.interface;
      const path = data.path;

      if (eventInterface === 'xyz.openbmc_project.State.Host') {
        const { properties: { CurrentHostState } = {} } = data;
        store.commit('global/setHostStatus', CurrentHostState);
      } else if (path === '/xyz/openbmc_project/logging') {
        store.dispatch('eventLog/getEventLogData');
      }
      // 2.5 sec debounce to avoid making multiple consecutive
      // GET requests since log related server messages seem to
      // come in clusters
    }, 2500);
  };

  store.subscribe(({ type }) => {
    if (type === 'authentication/authSuccess') {
      initWebSocket();
    }
    if (type === 'authentication/logout') {
      if (ws) ws.close();
    }
  });

  if (store.getters['authentication/isLoggedIn']) initWebSocket();
};

export default WebSocketPlugin;
