/**
 * api Interceptor
 *
 * @module app/common/services/apiInterceptor
 * @exports apiInterceptor
 * @name apiInterceptor

 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.services').service('apiInterceptor', [
    '$q', '$rootScope', 'dataService', '$location',
    function($q, $rootScope, dataService, $location) {
      return {
        'request': function(config) {
          dataService.loading = true;
          // If caller has not defined a timeout, set to default of 20s
          if (config.timeout == null) {
            config.timeout = 20000;
          }
          return config;
        },
        'response': function(response) {
          dataService.loading = false;

          // not interested in template requests
          if (!/^https?\:/i.test(response.config.url)) {
            return response;
          }

          dataService.last_updated = new Date();
          if (!response) {
            dataService.server_unreachable = true;
          } else {
            dataService.server_unreachable = false;
          }

          if (response && response.status == 'error' &&
              dataService.path != '/login') {
            $rootScope.$emit('timedout-user', {});
          }

          return response;
        },
        'responseError': function(rejection) {
          if (dataService.ignoreHttpError === false) {
            // If unauthorized, log out
            if (rejection.status == 401) {
              if (dataService.path != '/login') {
                $rootScope.$emit('timedout-user', {});
              }
            } else if (rejection.status == 403) {
              // TODO: when permission role mapping ready, remove
              // this global redirect and handle forbidden
              // requests in context of user action
              if (dataService.path != '/login') {
                $location.url('/unauthorized');
              }
            } else if (rejection.status == -1) {
              dataService.server_unreachable = true;
            }

            dataService.loading = false;
          }
          return $q.reject(rejection);
        }
      };
    }
  ]);
})(window.angular);
