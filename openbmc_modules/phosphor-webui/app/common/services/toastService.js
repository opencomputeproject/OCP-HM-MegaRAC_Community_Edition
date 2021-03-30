/**
 * data service
 *
 * @module app/common/services/toastService
 * @exports toastService
 * @name toastService

 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.services').service('toastService', [
    'ngToast', '$sce',
    function(ngToast, $sce) {
      function initToast(
          type = 'create', title = '', message = '', dismissOnTimeout = false) {
        const iconStatus = type === 'success' ?
            'on' :
            type === 'danger' ? 'error' : type === 'warning' ? 'warn' : null;
        const content = $sce.trustAsHtml(`
          <div role="alert" class="alert-content-container">
            <status-icon ng-if="${iconStatus !== null}"
                         status="${iconStatus}"
                         class="status-icon">
            </status-icon>
            <div class="alert-content">
              <h2 class="alert-content__header">${title}</h2>
              <p class="alert-content__body">${message}</p>
            </div>
          </div>`);
        ngToast[type]({content, dismissOnTimeout, compileContent: true});
      };

      this.error = function(message) {
        initToast('danger', 'Error', message);
      };

      this.success = function(message) {
        initToast('success', 'Success!', message, true);
      };

      this.warn = function(message) {
        initToast('warning', 'Warning', message);
      };

      this.info = function(title, message) {
        initToast('info', title, message);
      };
    }
  ]);
})(window.angular);
