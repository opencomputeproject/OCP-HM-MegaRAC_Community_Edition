import {Terminal} from 'xterm';
import style from 'xterm/dist/xterm.css';
import * as attach from 'xterm/lib/addons/attach/attach';
import * as fit from 'xterm/lib/addons/fit/fit';
var configJSON = require('../../../config.json');
if (configJSON.keyType == 'VT100+') {
  var vt100PlusKey = require('./vt100plus');
}

var customKeyHandlers = function(ev) {
  if (configJSON.keyType == 'VT100+') {
    return vt100PlusKey.customVT100PlusKey(ev, this);
  }
  return true;
};

function measureChar(term) {
  var span = document.createElement('span');
  var fontFamily = 'courier-new';
  var fontSize = 15;
  var rect;

  span.textContent = 'W';
  try {
    fontFamily = term.getOption('fontFamily');
    fontSize = term.getOption('fontSize');
  } catch (err) {
    console.log('get option failure');
  }
  span.style.fontFamily = fontFamily;
  span.style.fontSize = fontSize + 'px';
  document.body.appendChild(span);
  rect = span.getBoundingClientRect();
  document.body.removeChild(span);
  return rect;
}

/*
TextEncoder/TextDecoder does not support IE.
Use text-encoding instead in IE
More detail at https://caniuse.com/#feat=textencoder
*/
import {TextDecoder} from 'text-encoding';
if (!window['TextDecoder']) {
  window['TextDecoder'] = TextDecoder;
}

window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('serialConsole', [
    function() {
      return {
        'restrict': 'E',
        'template': require('./serial-console.html'),
        'scope': {'path': '=', 'showTabBtn': '=?'},
        'controller': [
          '$scope', '$cookies', '$window', 'dataService', '$element',
          function($scope, $cookies, $window, dataService, $element) {
            $scope.dataService = dataService;

            // See https://github.com/xtermjs/xterm.js/ for available xterm
            // options

            Terminal.applyAddon(attach);  // Apply the `attach` addon
            Terminal.applyAddon(fit);     // Apply the `fit` addon

            var border = 10;
            var term = new Terminal();
            // Should be a reference to <div id="terminal"></div>
            var terminal = $element[0].firstElementChild.firstElementChild;
            var customConsole;
            var charSize;
            var termContainer;

            term.open(terminal);
            customConsole = configJSON.customConsoleDisplaySize;

            if (customConsole != null) {
              charSize = measureChar(term);
              termContainer = document.getElementById('term-container');
              if (termContainer != null) {
                if (customConsole.width) {
                  termContainer.style.width =
                      (charSize.width * customConsole.width + border) + 'px';
                }
                if (customConsole.height) {
                  terminal.style.height =
                      (charSize.height * customConsole.height + border) + 'px';
                }
              }
            }
            term.fit();
            if (configJSON.customKeyEnable == true) {
              term.attachCustomKeyEventHandler(customKeyHandlers);
            }
            var SOL_THEME = {
              background: '#19273c',
              cursor: 'rgba(83, 146, 255, .5)',
              scrollbar: 'rgba(83, 146, 255, .5)'
            };
            term.setOption('theme', SOL_THEME);
            var hostname = dataService.getHost().replace('https://', '');
            var host = 'wss://' + hostname + '/console0';
            var token = $cookies.get('XSRF-TOKEN');
            try {
              var ws = new WebSocket(host, [token]);
              term.attach(ws);
              ws.onopen = function() {
                console.log('websocket opened');
              };
              ws.onclose = function(event) {
                console.log(
                    'websocket closed. code: ' + event.code +
                    ' reason: ' + event.reason);
              };
            } catch (error) {
              console.log(JSON.stringify(error));
            }
            $scope.openTerminalWindow = function() {
              $window.open(
                  '#/server-control/remote-console-window',
                  'Remote Console Window',
                  'directories=no,titlebar=no,toolbar=no,location=no,status=no,menubar=no,scrollbars=no,resizable=yes,width=600,height=550');
            };
          }
        ]
      };
    }
  ]);
})(window.angular);
