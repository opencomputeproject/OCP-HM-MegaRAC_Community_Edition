window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('syslogFilter', [
    'APIUtils',
    function(APIUtils) {
      return {
        'restrict': 'E',
        'template': require('./syslog-filter.html'),
        'controller': [
          '$rootScope', '$scope', 'dataService', '$location',
          function($rootScope, $scope, dataService, $location) {
            $scope.dataService = dataService;

            $scope.toggleSeverityAll = function() {
              $scope.selectedSeverityList = [];
            };

            $scope.toggleSeverity = function(severity) {
              if (severity == 'All') {
                $scope.selectedSeverityList = [];
                return;
              }

              var index = $scope.selectedSeverityList.indexOf(severity);
              if (index > -1) {
                $scope.selectedSeverityList.splice(index, 1);
              } else {
                $scope.selectedSeverityList.push(severity);
              }
              if ($scope.selectedSeverityList.length >=
                  ($scope.severityList.length - 1)) {
                $scope.selectedSeverityList = [];
              }
            };

            $scope.selectType = function(type) {
              $scope.selectedType = type;
              $scope.typeFilter = false;
            };
          }
        ]
      };
    }
  ]);
})(window.angular);
