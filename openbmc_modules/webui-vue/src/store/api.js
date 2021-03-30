import Axios from 'axios';
import router from '../router';
import store from '@/store';

const api = Axios.create({
  withCredentials: true
});

api.interceptors.response.use(undefined, error => {
  let response = error.response;

  // TODO: Provide user with a notification and way to keep system active
  if (response.status == 401) {
    if (response.config.url != '/login') {
      window.location = '/login';
      // Commit logout to remove XSRF-TOKEN cookie
      store.commit('authentication/logout');
    }
  }

  if (response.status == 403) {
    if (router.history.current.name === 'unauthorized') {
      // Check if current router location is unauthorized
      // to avoid NavigationDuplicated errors.
      // The router throws an error if trying to push to the
      // same/current router location.
      return;
    }
    router.push({ name: 'unauthorized' });
  }

  return Promise.reject(error);
});

export default {
  get(path) {
    return api.get(path);
  },
  delete(path, payload) {
    return api.delete(path, payload);
  },
  post(path, payload, config) {
    return api.post(path, payload, config);
  },
  patch(path, payload) {
    return api.patch(path, payload);
  },
  put(path, payload) {
    return api.put(path, payload);
  },
  all(promises) {
    return Axios.all(promises);
  },
  spread(callback) {
    return Axios.spread(callback);
  }
};

export const getResponseCount = responses => {
  let successCount = 0;
  let errorCount = 0;

  responses.forEach(response => {
    if (response instanceof Error) errorCount++;
    else successCount++;
  });

  return {
    successCount,
    errorCount
  };
};
