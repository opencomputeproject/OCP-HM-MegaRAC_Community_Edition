/**
 * A module for the Profile Settings page
 *
 * @module app/profile-settings/index
 * @exports app/profile-settings/index
 */

window.angular && (function(angular) {
  'use strict';

  angular
      .module('app.profileSettings', ['ngRoute', 'app.common.services'])
      // Route configuration
      .config([
        '$routeProvider',
        function($routeProvider) {
          $routeProvider.when('/profile-settings', {
            'template':
                require('./controllers/profile-settings-controller.html'),
            'controller': 'profileSettingsController',
            authenticated: true
          })
        }
      ]);
})(window.angular);
