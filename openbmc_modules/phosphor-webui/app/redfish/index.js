/**
 * A module for redfish
 *
 * @module app/redfish/index
 * @exports app/redfish/index
 */

window.angular && (function(angular) {
  'use strict';

  angular
      .module('app.redfish', ['ngRoute', 'app.redfish'])
      // Route configuration
      .config([
        '$routeProvider',
        function($routeProvider) {
          $routeProvider.when('/redfish/:path*/', {
            'template': require('./controllers/redfish-controller.html'),
            'controller': 'redfishController',
            authenticated: true
          });
        }
      ]);
})(window.angular);
