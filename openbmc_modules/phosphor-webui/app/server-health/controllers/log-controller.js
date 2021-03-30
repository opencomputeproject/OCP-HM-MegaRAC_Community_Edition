/**
 * Controller for log
 *
 * @module app/serverHealth
 * @exports logController
 * @name logController
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
      .controller('logController', [
        '$scope', '$window', 'APIUtils', 'dataService', 'Constants',
        '$routeParams', '$filter',
        function(
            $scope, $window, APIUtils, dataService, Constants, $routeParams,
            $filter) {
          $scope.dataService = dataService;
          $scope.logs = [];
          $scope.filteredLogs = [];
          $scope.tmz = '';
          $scope.itemsPerPage = Constants.PAGINATION.LOG_ITEMS_PER_PAGE;
          $scope.loading = false;
          var expandedSelectedIdOnce = false;

          var sensorType = $routeParams.type;
          var eventId = $routeParams.id;

          // priority buttons
          $scope.selectedSeverity =
              {all: true, low: false, medium: false, high: false};

          if (sensorType == 'high') {
            $scope.selectedSeverity.all = false;
            $scope.selectedSeverity.high = true;
          }

          $scope.selectedStatus = {all: true, resolved: false};

          $scope.customSearch = '';
          $scope.searchItems = [];
          $scope.selectedEvents = [];

          if (eventId) {
            $scope.customSearch = '#' + eventId;
            $scope.searchItems.push('#' + eventId);
          }

          $scope.loadLogs = function() {
            $scope.loading = true;
            APIUtils.getLogs().then(function(result) {
              if (eventId && expandedSelectedIdOnce == false) {
                var log = result.data.filter(function(item) {
                  return item.Id == eventId;
                });

                if (log.length) {
                  log[0].meta = true;
                }
                expandedSelectedIdOnce = true;
              }
              dataService.updateServerHealth(result.data);
              $scope.logs = result.data;
              $scope.originalData = result.original;
              $scope.loading = false;
            });
          };
          $scope.jsonData = function(data) {
            return JSON.stringify(data);
          };

          $scope.filterBySeverity = function(log) {
            if ($scope.selectedSeverity.all) return true;

            return (
                (log.severity_flags.low && $scope.selectedSeverity.low) ||
                (log.severity_flags.medium && $scope.selectedSeverity.medium) ||
                (log.severity_flags.high && $scope.selectedSeverity.high));
          };

          $scope.filterByStatus = function(log) {
            if ($scope.selectedStatus.all) return true;
            return (
                (log.Resolved && $scope.selectedStatus.resolved) ||
                (!log.Resolved && !$scope.selectedStatus.resolved));
          };

          $scope.filterByDate = function(log) {
            var endDate;
            if ($scope.end_date &&
                typeof $scope.end_date.getTime === 'function') {
              endDate = new Date($scope.end_date.getTime());
              endDate.setTime(endDate.getTime() + 86399000);
            }

            if ($scope.start_date && endDate) {
              return (
                  log.Timestamp >= $scope.start_date &&
                  log.Timestamp <= endDate);
            } else {
              return true;
            }
          };

          $scope.filterBySearchTerms = function(log) {
            const searchableProperties = [
              'eventID', 'severity_code', 'description', 'priority',
              'additional_data', 'type', 'logId'
            ];
            let matched = false;

            if (!$scope.searchItems.length) return true;
            for (const searchTerm of $scope.searchItems) {
              if (matched) {
                break;
              }
              for (const prop of searchableProperties) {
                const propVal = log[prop];
                if (propVal &&
                    propVal.toLowerCase().indexOf(searchTerm) !== -1) {
                  matched = true;
                  break;
                }
              }
            }
            return matched;
          };

          $scope.addSearchItem = function(searchTerms) {
            var terms = searchTerms.split(' ');
            terms.forEach(function(searchTerm) {
              if ($scope.searchItems.indexOf(searchTerm) == -1) {
                $scope.searchItems.push(searchTerm.toLowerCase());
              }
            });
          };

          $scope.clearSearchItem = function(searchTerm) {
            $scope.searchItems = [];
          };

          $scope.removeSearchItem = function(searchTerm) {
            var termIndex = $scope.searchItems.indexOf(searchTerm);

            if (termIndex > -1) {
              $scope.searchItems.splice(termIndex, 1);
            }
          };

          $scope.$watch('all', function() {
            $scope.filteredLogs.forEach(function(item) {
              item.selected = $scope.all;
            });
          });

          function updateExportData() {
            $scope.export_name = ($scope.selectedEvents.length == 1) ?
                $scope.selectedEvents[0].Id + '.json' :
                'export.json';
            var data = {};
            $scope.selectedEvents.forEach(function(item) {
              data[item.data.key] = item.data.value;
            });
            $scope.export_data = JSON.stringify(data);
          }

          $scope.accept = function() {
            APIUtils.deleteLogs($scope.selectedEvents).then(function() {
              $scope.confirm = false;
              $scope.loadLogs();
            });
          };

          $scope.resolve = function() {
            var events = $scope.selectedEvents.filter(function(item) {
              return item.Resolved != 1;
            });

            if (!events.length) return;

            APIUtils.resolveLogs(events).then(
                function(data) {
                  events.forEach(function(item) {
                    item.Resolved = 1;
                  });
                },
                function(error) {
                  // TODO: Show error to user
                  console.log(JSON.stringify(error));
                });
          };

          $scope.$watch('logs', function() {
            $scope.selectedEvents = $scope.logs.filter(function(item) {
              return item.selected;
            });
            updateExportData();
          }, true);

          $scope.loadLogs();
        }
      ]);
})(angular);
