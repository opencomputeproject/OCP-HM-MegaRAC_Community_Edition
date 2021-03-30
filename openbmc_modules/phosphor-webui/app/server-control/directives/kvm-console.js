/**
 * Directive for KVM (Kernel-based Virtual Machine)
 *
 * @module app/serverControl
 * @exports kvmConsole
 * @name kvmConsole
 */

import RFB from '@novnc/novnc/core/rfb.js';

window.angular && (function(angular) {
  'use strict';

  angular.module('app.serverControl').directive('kvmConsole', [
    '$log', '$cookies', '$location',
    function($log, $cookies, $location) {
      return {
        restrict: 'E', template: require('./kvm-console.html'),
            scope: {newWindowBtn: '=?'}, link: function(scope, element) {
              var rfb;

              element.on('$destroy', function() {
                if (rfb) {
                  rfb.disconnect();
                }
              });

              function sendCtrlAltDel() {
                rfb.sendCtrlAltDel();
                return false;
              };

              function connected(e) {
                $log.debug('RFB Connected');
              }

              function disconnected(e) {
                $log.debug('RFB disconnected');
              }

              var host = $location.host();
              var port = $location.port();
              var target = element[0].firstElementChild;
              try {
                var token = $cookies.get('XSRF-TOKEN');
                rfb = new RFB(
                    target, 'wss://' + host + '/kvm/0',
                    {'wsProtocols': [token]});
                rfb.addEventListener('connect', connected);
                rfb.addEventListener('disconnect', disconnected);
              } catch (exc) {
                $log.error(exc);
                updateState(
                    null, 'fatal', null,
                    'Unable to create RFB client -- ' + exc);
                return;  // don't continue trying to connect
              };

              scope.openWindow = function() {
                window.open(
                    '#/server-control/kvm-window', 'Kvm Window',
                    'directories=no,titlebar=no,toolbar=no,location=no,status=no,menubar=no,scrollbars=no,resizable=yes,width=1125,height=900');
              };
            }
      }
    }
  ]);
})(window.angular);
