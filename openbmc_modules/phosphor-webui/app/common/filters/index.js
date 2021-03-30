window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.filters', [])
      .filter(
          'unResolvedCount',
          function() {
            return function(data) {
              data = data.filter(function(item) {
                return item.Resolved == 0;
              });
              return data.length;
            }
          })
      .filter(
          'quiescedToError',
          function() {
            return function(state) {
              if (state.toLowerCase() == 'quiesced') {
                return 'Error';
              } else {
                return state;
              }
            }
          })
      .filter('localeDate', function() {
        return function(timestamp, utc = false) {
          var dt = new Date(timestamp);
          if (isNaN(dt)) {
            return 'not available';
          }

          const ro = Intl.DateTimeFormat().resolvedOptions();
          var tz = 'UTC';
          if (!utc) {
            tz = ro.timeZone;
          }

          // Examples:
          //   "Dec 3, 2018 11:35:01 AM CST" for en-US at 'America/Chicago'
          //   "Dec 3, 2018 5:35:01 PM UTC" for en-US at 'UTC'
          //   "Dec 3, 2018 17:35:01 GMT" for en-GB at 'Europe/London'
          //   "Dec 3, 2018 20:35:01 GMT+3" for ru-RU at 'Europe/Moscow'
          //   "Dec 3, 2018 17:35:01 UTC" for ru-RU at 'UTC'
          return dt.toLocaleDateString('en-US', {
            timeZone: tz,
            month: 'short',
            year: 'numeric',
            day: 'numeric'
          }) + ' ' +
              dt.toLocaleTimeString(
                  ro.locale, {timeZone: tz, timeZoneName: 'short'});
        }
      });
})(window.angular);
