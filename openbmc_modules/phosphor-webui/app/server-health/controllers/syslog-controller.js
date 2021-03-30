/**
 * Controller for log
 *
 * @module app/serverHealth
 * @exports sysLogController
 * @name sysLogController
 */

window.angular && (function(angular) {
  'use strict';
  angular.module('app.serverHealth')
      .config([
        'paginationTemplateProvider',
        function(paginationTemplateProvider) {
          paginationTemplateProvider.setString(
              require('../../common/directives/dirPagination.tpl.html'));
        }
      ])
      .controller('sysLogController', [
        '$scope', 'APIUtils', 'Constants',
        function($scope, APIUtils, Constants) {
          $scope.itemsPerPage = Constants.PAGINATION.LOG_ITEMS_PER_PAGE;
          $scope.loading = true;
          $scope.sysLogs = [];
          $scope.customSearch = '';
          $scope.searchTerms = [];
          $scope.sortKey = 'Id';
          $scope.showLogDropdown = false;
          $scope.recordTypeList =
              ['SEL', 'Event', 'Oem'];        // From Redfish specification.
          $scope.selectedRecordType = 'SEL';  // Default Select to SEL.
          $scope.typeFilter = false;
          $scope.selectedSeverityList = [];
          $scope.severityList = ['All', 'Critical', 'Warning', 'Ok'];
          $scope.filterTypes = [];
          $scope.selectedType = 'All';

          $scope.selectRecordType = function(recordType) {
            $scope.selectedRecordType = recordType;
            $scope.showLogDropdown = false;
            $scope.filterTypes = [];

            APIUtils.getSystemLogs(recordType)
                .then(
                    function(res) {
                      $scope.sysLogs = res;
                      $scope.filterTypes.push('All');
                      $scope.sysLogs.forEach(function(log) {
                        if ($scope.filterTypes.indexOf(log.SensorType) < 0) {
                          $scope.filterTypes.push(log.SensorType);
                        }
                      });
                    },
                    function(error) {
                      console.log(JSON.stringify(error));
                    })
                .finally(function() {
                  $scope.loading = false;
                });
          };

          $scope.clearSystemLogEntries = function(selectedRecordType) {
            $scope.confirm = false;
            APIUtils.clearSystemLogs(selectedRecordType)
                .then(
                    function(res) {
                      console.log(JSON.stringify(res));
                    },
                    function(error) {
                      console.log(JSON.stringify(error));
                    })
                .finally(function() {
                  $scope.selectRecordType($scope.selectedRecordType);
                });
          };

          $scope.sortBy = function(keyname, isReverse) {
            $scope.sortKey = keyname;
            $scope.reverse = isReverse;
          };

          $scope.clear = function() {
            $scope.customSearch = '';
            $scope.searchTerms = [];
          };

          $scope.doSearchOnEnter = function(event) {
            var search =
                $scope.customSearch.replace(/^\s+/g, '').replace(/\s+$/g, '');
            if (event.keyCode === 13 && search.length >= 2) {
              $scope.searchTerms = $scope.customSearch.split(' ');
            } else {
              if (search.length == 0) {
                $scope.searchTerms = [];
              }
            }
          };

          $scope.doSearchOnClick = function() {
            var search =
                $scope.customSearch.replace(/^\s+/g, '').replace(/\s+$/g, '');
            if (search.length >= 2) {
              $scope.searchTerms = $scope.customSearch.split(' ');
            } else {
              if (search.length == 0) {
                $scope.searchTerms = [];
              }
            }
          };

          $scope.filterBySearchTerms = function(log) {
            if (!$scope.searchTerms.length) {
              return true;
            }

            for (var i = 0, length = $scope.searchTerms.length; i < length;
                 i++) {
              // TODO: Form it while getting data
              var search_text = log.Id + ' ' + log.Name.toLowerCase() + ' ' +
                  log.Message.toLowerCase();
              if (search_text.indexOf($scope.searchTerms[i].toLowerCase()) ==
                  -1)
                return false;
            }
            return true;
          };

          $scope.filterBySeverity = function(log) {
            if ($scope.selectedSeverityList.length == 0) {
              return true;
            }

            return ($scope.selectedSeverityList.indexOf(log.Severity) > -1);
          };

          $scope.filterByType = function(log) {
            if ($scope.selectedType == 'All') {
              return true;
            }

            return (($scope.selectedType == log.SensorType));
          };

          $scope.filterByDate = function(log) {
            var logDate = new Date(log.Created);
            if ($scope.start_date && $scope.end_date) {
              return (
                  logDate >= $scope.start_date && logDate <= $scope.end_date);
            } else {
              return true;
            }
          };

          setTimeout($scope.selectRecordType($scope.selectedRecordType), 2000);
        }
      ]);
})(angular);
