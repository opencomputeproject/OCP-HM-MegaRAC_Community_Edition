/**
 * Controller for date-time
 *
 * @module app/configuration
 * @exports dateTimeController
 * @name dateTimeController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.configuration').controller('dateTimeController', [
    '$scope', 'APIUtils', '$route', '$q', 'toastService', '$timeout',
    function($scope, APIUtils, $route, $q, toastService, $timeout) {
      $scope.bmc = {};
      // Only used when the owner is "Split"
      $scope.host = {};
      $scope.ntp = {servers: []};
      $scope.time = {mode: '', owner: ''};
      // Possible time owners
      // https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Time/Owner.interface.yaml#L13
      $scope.timeOwners = ['BMC', 'Host', 'Both', 'Split'];
      $scope.loading = true;
      var timePath = '/xyz/openbmc_project/time/';

      var getTimePromise = APIUtils.getTime().then(
          function(data) {
            // The time is returned as Epoch microseconds convert to
            // milliseconds.
            if (data.data[timePath + 'bmc'] &&
                data.data[timePath + 'bmc'].hasOwnProperty('Elapsed')) {
              $scope.bmc.date =
                  new Date(data.data[timePath + 'bmc'].Elapsed / 1000);
              // Don't care about milliseconds and don't want them displayed
              $scope.bmc.date.setMilliseconds(0);

              // Examples:
              //   Central Standard Time (UTC-06:00)
              //   Moscow Standard Time (UTC+03:00)
              $scope.bmc.timezone = getUserTimezone($scope.bmc.date) + ' ' +
                  createOffset($scope.bmc.date);
            }
            if (data.data[timePath + 'host'] &&
                data.data[timePath + 'host'].hasOwnProperty('Elapsed')) {
              $scope.host.date =
                  new Date(data.data[timePath + 'host'].Elapsed / 1000);
              $scope.host.date.setMilliseconds(0);
              $scope.host.timezone = getUserTimezone($scope.bmc.date) + ' ' +
                  createOffset($scope.bmc.date);
            }
            if (data.data[timePath + 'owner'] &&
                data.data[timePath + 'owner'].hasOwnProperty('TimeOwner')) {
              $scope.time.owner =
                  data.data[timePath + 'owner'].TimeOwner.split('.').pop();
            }
            if (data.data[timePath + 'sync_method'] &&
                data.data[timePath + 'sync_method'].hasOwnProperty(
                    'TimeSyncMethod')) {
              $scope.time.mode = data.data[timePath + 'sync_method']
                                     .TimeSyncMethod.split('.')
                                     .pop();
            }
          },
          function(error) {
            console.log(JSON.stringify(error));
          });

      var getNTPPromise = APIUtils.getNTPServers().then(
          function(data) {
            $scope.ntp.servers = data.data;
          },
          function(error) {
            console.log(JSON.stringify(error));
          });

      var promises = [getTimePromise, getNTPPromise];

      $q.all(promises).finally(function() {
        $scope.loading = false;
      });

      /**
       * https://github.com/openbmc/phosphor-time-manager/blob/master/README.md#special-note-on-changing-ntp-setting
       * When time mode is initially set to Manual from NTP,
       * NTP service is disabled and the NTP service is
       * stopping but not stopped, setting time will return an error.
       * There are no responses from backend to notify when NTP is stopped.
       * To work around, a timeout is set to allow NTP to fully stop
       * TODO: remove timeout if backend solves
       * https://github.com/openbmc/openbmc/issues/3459
       */
      $scope.saveDateTimeSettings = function() {
        $scope.loading = true;
        if ($scope.time.mode == 'Manual' || $scope.time.owner == 'Split') {
          setTimeMode()
              .then(setTimeOwner)
              .then(setNTPServers)
              .then($timeout(setDateTime, 20000));
        } else {
          setTimeMode()
              .then(setTimeOwner)
              .then(setNTPServers)
              .then(
                  function() {
                    toastService.success('Date and time settings saved');
                  },
                  function(errors) {
                    console.log(JSON.stringify(errors));
                    toastService.error(
                        'Date and time settings could not be saved');
                  })
              .finally(function() {
                $scope.loading = false;
              });
        }
      };

      const setDateTime = function() {
        var manualPromises = [];
        if ($scope.time.mode == 'Manual') {
          // If owner is 'Split' set both.
          // If owner is 'Host' set only it.
          if ($scope.time.owner != 'Host') {
            manualPromises.push(setBMCTime($scope.bmc.date.getTime() * 1000));
          }
          // Even though we are setting Host time, we are setting from
          // the BMC date and time fields labeled "BMC and Host Time"
          // currently.
          if ($scope.time.owner == 'Host') {
            manualPromises.push(setHostTime($scope.bmc.date.getTime() * 1000));
          }
        }
        // Set the Host if Split even if NTP. In split mode, the host has
        // its own date and time field set from it.
        if ($scope.time.owner == 'Split') {
          manualPromises.push(setHostTime($scope.host.date.getTime() * 1000));
        }

        $q.all(manualPromises)
            .then(
                function() {
                  toastService.success('Date and time settings saved');
                },
                function(errors) {
                  console.log(JSON.stringify(errors));
                  toastService.error(
                      'Date and time settings could not be saved');
                })
            .finally(function() {
              $scope.loading = false;
            });
      };

      $scope.refresh = function() {
        $route.reload();
      };

      $scope.addNTPField = function() {
        $scope.ntp.servers.push('');
      };

      $scope.removeNTPField = function(index) {
        $scope.ntp.servers.splice(index, 1);
      };

      function setNTPServers() {
        // Remove any empty strings from the array. If the
        // user doesn't fill out the field, we don't want to add.
        $scope.ntp.servers = $scope.ntp.servers.filter(Boolean);

        return APIUtils.setNTPServers($scope.ntp.servers);
      }

      function setTimeMode() {
        return APIUtils.setTimeMode(
            'xyz.openbmc_project.Time.Synchronization.Method.' +
            $scope.time.mode);
      }

      function setTimeOwner() {
        return APIUtils.setTimeOwner(
            'xyz.openbmc_project.Time.Owner.Owners.' + $scope.time.owner);
      }

      function setBMCTime(time) {
        // Add the separate date and time objects and convert to Epoch time in
        // microseconds.
        return APIUtils.setBMCTime(time);
      }

      function setHostTime(time) {
        // Add the separate date and time objects and convert to Epoch time
        // microseconds.
        return APIUtils.setHostTime(time);
      }
      function createOffset(date) {
        // https://stackoverflow.com/questions/9149556/how-to-get-utc-offset-in-javascript-analog-of-timezoneinfo-getutcoffset-in-c
        var sign = (date.getTimezoneOffset() > 0) ? '-' : '+';
        var offset = Math.abs(date.getTimezoneOffset());
        var hours = pad(Math.floor(offset / 60));
        var minutes = pad(offset % 60);
        return '(UTC' + sign + hours + ':' + minutes + ')';
      }
      function getUserTimezone(date) {
        const ro = Intl.DateTimeFormat().resolvedOptions();
        // A safe, easy way to get the timezone (e.g. Central Standard Time) is
        // to subtract the time string without a timezone from the time string
        // with a timezone.
        // Hardcoded to 'en-US' so all timezones are displayed in English
        // (e.g. Moscow Standard Time).
        var ret = date.toLocaleTimeString('en-US', {timeZoneName: 'long'})
                      .replace(date.toLocaleTimeString('en-US'), '')
                      .trim();
        // Do not return GMT+/-offset.
        if (ret.indexOf('GMT') >= 0) {
          return '';
        }
        return ret;
      }
      function pad(value) {
        return value < 10 ? '0' + value : value;
      }
    }
  ]);
})(angular);
