window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('logSearchControl', [
    'APIUtils',
    function(APIUtils) {
      return {
        'restrict': 'E',
        'template': require('./log-search-control.html'),
        'controller': [
          '$rootScope', '$scope', 'dataService', '$location',
          function($rootScope, $scope, dataService, $location) {
            $scope.dataService = dataService;
            $scope.doSearchOnEnter = function(event) {
              var search =
                  $scope.customSearch.replace(/^\s+/g, '').replace(/\s+$/g, '');
              if (event.keyCode === 13 && search.length >= 2) {
                $scope.clearSearchItem();
                $scope.addSearchItem(search);
              } else {
                if (search.length == 0) {
                  $scope.clearSearchItem();
                }
              }
            };

            $scope.clear = function() {
              $scope.customSearch = '';
              $scope.clearSearchItem();
            };

            $scope.doSearchOnClick = function() {
              var search =
                  $scope.customSearch.replace(/^\s+/g, '').replace(/\s+$/g, '');
              if (search.length >= 2) {
                $scope.clearSearchItem();
                $scope.addSearchItem(search);
              } else {
                if (search.length == 0) {
                  $scope.clearSearchItem();
                }
              }
            };
          }
        ]
      };
    }
  ]);
})(window.angular);
