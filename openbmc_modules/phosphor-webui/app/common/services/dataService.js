/**
 * data service
 *
 * @module app/common/services/dataService
 * @exports dataService
 * @name dataService

 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.services').service('dataService', [
    'Constants',
    function(Constants) {
      this.server_health = Constants.SERVER_HEALTH.unknown;
      this.server_state = 'Unreachable';
      this.LED_state = Constants.LED_STATE_TEXT.off;
      this.last_updated = new Date();

      this.loading = false;
      this.server_unreachable = false;
      this.showNavigation = false;
      this.bodyStyle = {};
      this.path = '';

      this.hostname = '';
      this.mac_address = '';
      this.defaultgateway = '';

      this.ignoreHttpError = false;
      this.systemName = '';

      this.configJson = require('../../../config.json');

      this.getServerId = function() {
        return this.host.replace(/^https?\:\/\//ig, '');
      };

      this.reloadServerId = function() {
        this.server_id = this.getServerId();
      };

      this.getHost = function() {
        if (sessionStorage.getItem(
                Constants.API_CREDENTIALS.host_storage_key) !== null) {
          return sessionStorage.getItem(
              Constants.API_CREDENTIALS.host_storage_key);
        } else {
          return Constants.API_CREDENTIALS.default_protocol + '://' +
              window.location.hostname +
              (window.location.port ? ':' + window.location.port : '');
        }
      };

      this.setHost = function(hostWithPort) {
        hostWithPort = hostWithPort.replace(/^https?\:\/\//ig, '');
        var hostURL =
            Constants.API_CREDENTIALS.default_protocol + '://' + hostWithPort;
        sessionStorage.setItem(
            Constants.API_CREDENTIALS.host_storage_key, hostURL);
        this.host = hostURL;
        this.reloadServerId();
      };

      this.getUser = function() {
        return sessionStorage.getItem('LOGIN_ID');
      };

      this.host = this.getHost();
      this.server_id = this.getServerId();

      this.setNetworkInfo = function(data) {
        this.hostname = data.hostname;
        this.defaultgateway = data.defaultgateway;
        this.mac_address = data.mac_address;
      };

      this.setPowerOnState = function() {
        this.server_state = Constants.HOST_STATE_TEXT.on;
      };

      this.setPowerOffState = function() {
        this.server_state = Constants.HOST_STATE_TEXT.off;
      };

      this.setErrorState = function() {
        this.server_state = Constants.HOST_STATE_TEXT.error;
      };

      this.setUnreachableState = function() {
        this.server_state = Constants.HOST_STATE_TEXT.unreachable;
      };

      this.updateServerHealth = function(logs) {
        // If any unresolved severity high logs are present, set server health
        // to critical. Else if any unresolved severity medium logs are present
        // set server health to warning.
        this.server_health = Constants.SERVER_HEALTH.good;
        for (var log of logs) {
          if (log.priority == 'High' && !log.Resolved) {
            this.server_health = Constants.SERVER_HEALTH.critical;
            return;
          } else if (log.priority == 'Medium' && !log.Resolved) {
            this.server_health = Constants.SERVER_HEALTH.warning;
          }
        }
      };

      this.setSystemName = function(sysName) {
        this.systemName = sysName;
      };
    }
  ]);
})(window.angular);
