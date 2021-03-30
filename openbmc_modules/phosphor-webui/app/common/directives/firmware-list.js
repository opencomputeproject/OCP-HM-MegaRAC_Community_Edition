window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('firmwareList', [
    'APIUtils',
    function(APIUtils) {
      return {
        'restrict': 'E',
        'template': require('./firmware-list.html'),
        'scope':
            {'title': '@', 'firmwares': '=', 'filterBy': '=', 'version': '='},
        'controller': [
          '$rootScope', '$scope', 'dataService', '$location', '$timeout',
          function($rootScope, $scope, dataService, $location, $timeout) {
            $scope.dataService = dataService;
            $scope.activate = function(imageId, imageVersion, imageType) {
              $scope.$parent.activateImage(imageId, imageVersion, imageType);
            };

            $scope.delete = function(imageId, imageVersion) {
              $scope.$parent.deleteImage(imageId, imageVersion);
            };

            $scope.changePriority = function(imageId, imageVersion, from, to) {
              $scope.$parent.changePriority(imageId, imageVersion, from, to);
            };

            $scope.toggleMoreDropdown = function(event, firmware) {
              firmware.extended.show = !firmware.extended.show;
              event.stopPropagation();
            };
          }
        ]
      };
    }
  ]);
})(window.angular);
