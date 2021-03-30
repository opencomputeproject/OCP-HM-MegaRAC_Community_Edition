/**
 * A module for the configuration
 *
 * @module app/configuration/index
 * @exports app/configuration/index
 */

window.angular && (function(angular) {
  'use strict';

  angular
      .module('app.configuration', ['ngRoute', 'app.common.services'])
      // Route configuration
      .config([
        '$routeProvider',
        function($routeProvider) {
          $routeProvider
              .when('/configuration/network', {
                'template': require('./controllers/network-controller.html'),
                'controller': 'networkController',
                authenticated: true
              })
              .when('/configuration/date-time', {
                'template': require('./controllers/date-time-controller.html'),
                'controller': 'dateTimeController',
                authenticated: true
              })
              .when('/configuration', {
                'template': require('./controllers/network-controller.html'),
                'controller': 'networkController',
                authenticated: true
              })
              .when('/configuration/snmp', {
                'template': require('./controllers/snmp-controller.html'),
                'controller': 'snmpController',
                authenticated: true
              })
              .when('/configuration/firmware', {
                'template': require('./controllers/firmware-controller.html'),
                'controller': 'firmwareController',
                authenticated: true
              });
        }
      ]);
})(window.angular);
