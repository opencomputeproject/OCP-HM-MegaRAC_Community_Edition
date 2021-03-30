import api from '../../api';
import Cookies from 'js-cookie';
import router from '../../../router';

const AuthenticationStore = {
  namespaced: true,
  state: {
    authError: false,
    cookie: Cookies.get('XSRF-TOKEN')
  },
  getters: {
    authError: state => state.authError,
    isLoggedIn: state => !!state.cookie,
    token: state => state.cookie
  },
  mutations: {
    authSuccess(state) {
      state.authError = false;
      state.cookie = Cookies.get('XSRF-TOKEN');
    },
    authError(state, authError = true) {
      state.authError = authError;
    },
    logout() {
      Cookies.remove('XSRF-TOKEN');
      localStorage.removeItem('storedUsername');
    }
  },
  actions: {
    login({ commit }, auth) {
      commit('authError', false);
      return api
        .post('/login', { data: auth })
        .then(() => commit('authSuccess'))
        .catch(error => {
          commit('authError');
          throw new Error(error);
        });
    },
    logout({ commit }) {
      api
        .post('/logout', { data: [] })
        .then(() => commit('logout'))
        .then(() => router.go('/login'))
        .catch(error => console.log(error));
    },
    async checkPasswordChangeRequired(_, username) {
      return await api
        .get(`/redfish/v1/AccountService/Accounts/${username}`)
        .then(({ data: { PasswordChangeRequired } }) => PasswordChangeRequired)
        .catch(error => console.log(error));
    }
  }
};

export default AuthenticationStore;
