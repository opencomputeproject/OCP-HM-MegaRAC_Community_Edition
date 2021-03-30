window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('logEvent', [
    'APIUtils',
    function(APIUtils) {
      return {
        'restrict': 'E',
        'template': require('./log-event.html'),
        'scope': {'event': '=', 'tmz': '=', 'multiSelected': '='},
        'controller': [
          '$rootScope', '$scope', 'dataService', '$location', '$timeout',
          function($rootScope, $scope, dataService, $location, $timeout) {
            $scope.dataService = dataService;
            $scope.copySuccess = function(event) {
              event.copied = true;
              $timeout(function() {
                event.copied = false;
              }, 5000);
            };
            $scope.copyFailed = function(err) {
              console.error('Error!', err);
            };
            $scope.resolveEvent = function(event) {
              APIUtils.resolveLogs([{Id: event.Id}])
                  .then(
                      function(data) {
                        event.Resolved = 1;
                      },
                      function(error) {
                        // TODO: Show error to user
                        console.log(JSON.stringify(error));
                      });
            };
            $scope.accept = function() {
              $scope.event.selected = true;
              $timeout(function() {
                $scope.$parent.accept();
              }, 10);
            };

            $scope.getTitle = function(event) {
              var title = event.type;
              if ((event.eventID != 'None') && (event.description != 'None')) {
                title = event.eventID + ': ' + event.description;
              }
              return title;
            };

            $scope.getAdditionalData = function(event) {
              var data = event.additional_data;
              // Stick the type into the additional data if it isn't
              // already in the title.
              if ($scope.getTitle(event).search(event.type) == -1) {
                data += '\nMESSAGE=' + event.type;
              }
              return data;
            };
            $scope.copyText = function(event) {
              return event.description + ' ' + event.additional_data;
            }
          }
        ]
      };
    }
  ]);
})(window.angular);
