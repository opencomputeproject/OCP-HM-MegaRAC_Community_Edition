window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('loader', function() {
    return {
      'restrict': 'E',
      'template': require('./loader.html'),
      scope: {loading: '='}
    };
  });
})(window.angular);
