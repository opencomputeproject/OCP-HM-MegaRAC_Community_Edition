window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('appHeader', [
    'APIUtils',
    function(APIUtils) {
      return {
        'restrict': 'E',
        'template': require('./app-header.html'),
        'scope': {'path': '='},
        'controller': [
          '$rootScope', '$cookies', '$scope', 'dataService', 'userModel',
          '$location', '$route',
          function(
              $rootScope, $cookies, $scope, dataService, userModel, $location,
              $route) {
            $scope.dataService = dataService;
            $scope.username = '';

            try {
              // Create a secure websocket with URL as /subscribe
              // TODO: Need to put in a generic APIUtils to avoid duplicate
              // controller
              var token = $cookies.get('XSRF-TOKEN');
              var ws = new WebSocket(
                  'wss://' + dataService.server_id + '/subscribe', [token]);
            } catch (error) {
              console.log('WebSocket', error);
            }

            if (ws !== undefined) {
              // Specify the required event details as JSON dictionary
              var data = JSON.stringify({
                'paths': ['/xyz/openbmc_project/state/host0'],
                'interfaces': ['xyz.openbmc_project.State.Host']
              });

              // Send the JSON dictionary data to host
              ws.onopen = function() {
                ws.send(data);
                console.log('host0 ws opened');
              };

              // Close the web socket
              ws.onclose = function() {
                console.log('host0 ws closed');
              };

              // Websocket event handling function which catches the
              // current host state
              ws.onmessage = function(evt) {
                // Parse the response (JSON dictionary data)
                var content = JSON.parse(evt.data);

                // Fetch the current server power state
                if (content.hasOwnProperty('properties') &&
                    content['properties'].hasOwnProperty('CurrentHostState')) {
                  // Refresh the host state and status
                  // TODO: As of now not using the current host state
                  // value for updating the data Service state rather
                  // using it to detect the command line state change.
                  // Tried different methods like creating a separate
                  // function, adding ws under $scope etc.. but auto
                  // refresh is not happening.
                  $scope.loadServerStatus();
                }
              };
            }

            $scope.loadServerHealth = function() {
              APIUtils.getLogs().then(function(result) {
                dataService.updateServerHealth(result.data);
              });
            };

            $scope.loadServerStatus = function() {
              if (!userModel.isLoggedIn()) {
                return;
              }
              APIUtils.getHostState().then(
                  function(status) {
                    if (status ==
                        'xyz.openbmc_project.State.Host.HostState.Off') {
                      dataService.setPowerOffState();
                    } else if (
                        status ==
                        'xyz.openbmc_project.State.Host.HostState.Running') {
                      dataService.setPowerOnState();
                    } else {
                      dataService.setErrorState();
                    }
                  },
                  function(error) {
                    console.log(error);
                  });
            };

            $scope.loadNetworkInfo = function() {
              if (!userModel.isLoggedIn()) {
                return;
              }
              APIUtils.getNetworkInfo().then(function(data) {
                dataService.setNetworkInfo(data);
              });
            };

            $scope.loadSystemName = function() {
              // Dynamically get ComputerSystems Name/serial
              // which differs across OEM's
              APIUtils.getRedfishSysName().then(
                  function(res) {
                    dataService.setSystemName(res);
                  },
                  function(error) {
                    console.log(JSON.stringify(error));
                  });
            };

            function loadData() {
              $scope.loadServerStatus();
              $scope.loadNetworkInfo();
              $scope.loadServerHealth();
              $scope.loadSystemName();
              $scope.username = dataService.getUser();
            }

            loadData();

            $scope.logout = function() {
              userModel.logout(function(status, error) {
                if (status) {
                  $location.path('/logout');
                } else {
                  console.log(error);
                }
              });
            };

            $scope.refresh = function() {
              // reload current page controllers and header
              loadData();
              $route.reload();
              // Add flash class to header timestamp on click of refresh
              var myEl =
                  angular.element(document.querySelector('.header__refresh'));
              myEl.addClass('flash');
              setTimeout(function() {
                myEl.removeClass('flash');
              }, 2000);
            };

            var loginListener =
                $rootScope.$on('user-logged-in', function(event, arg) {
                  loadData();
                });

            $scope.$on('$destroy', function() {
              loginListener();
            });
          }
        ]
      };
    }
  ]);
})(window.angular);
