/**
 * A module for the serverControl
 *
 * @module app/server-control/index
 * @exports app/server-control/index
 */

window.angular && (function(angular) {
  'use strict';

  angular
      .module('app.serverControl', ['ngRoute', 'app.common.services'])
      // Route configuration
      .config([
        '$routeProvider',
        function($routeProvider) {
          $routeProvider
              .when('/server-control/bmc-reboot', {
                'template': require('./controllers/bmc-reboot-controller.html'),
                'controller': 'bmcRebootController',
                authenticated: true
              })
              .when('/server-control/server-led', {
                'template': require('./controllers/server-led-controller.html'),
                'controller': 'serverLEDController',
                authenticated: true
              })
              .when('/server-control/power-operations', {
                'template':
                    require('./controllers/power-operations-controller.html'),
                'controller': 'powerOperationsController',
                authenticated: true
              })
              .when('/server-control/power-usage', {
                'template':
                    require('./controllers/power-usage-controller.html'),
                'controller': 'powerUsageController',
                authenticated: true
              })
              .when('/server-control/remote-console', {
                'template':
                    require('./controllers/remote-console-controller.html'),
                authenticated: true
              })
              .when('/server-control/remote-console-window', {
                'template': require(
                    './controllers/remote-console-window-controller.html'),
                'controller': 'remoteConsoleWindowController',
                authenticated: true
              })
              .when('/server-control/kvm', {
                'template': require('./controllers/kvm-controller.html'),
                authenticated: true
              })
              .when('/server-control/kvm-window', {
                'template': require('./controllers/kvm-window-controller.html'),
                'controller': 'kvmWindowController',
                authenticated: true
              })
              .when('/server-control/virtual-media', {
                'template':
                    require('./controllers/virtual-media-controller.html'),
                'controller': 'virtualMediaController',
                authenticated: true
              })
              .when('/server-control', {
                'template':
                    require('./controllers/power-operations-controller.html'),
                'controller': 'powerOperationsController',
                authenticated: true
              });
        }
      ]);
})(window.angular);
