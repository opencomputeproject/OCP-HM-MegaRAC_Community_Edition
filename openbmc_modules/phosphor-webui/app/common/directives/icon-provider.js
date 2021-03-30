/**
 * Directive to inline an svg icon
 *
 * To use–add an <icon> directive with a file attribute with
 * a value that corresponds to the desired svg file to inline
 * from the icons directory.
 *
 * Example: <icon file="icon-export.svg"></icon>
 *
 */
window.angular && ((angular) => {
  'use-strict';

  angular.module('app.common.directives').directive('icon', () => {
    return {
      restrict: 'E',
      link: (scope, element, attrs) => {
        const file = attrs.file || attrs.ngFile;
        if (file === undefined) {
          console.log('File name not provided for <icon> directive.')
          return;
        }
        const svg = require(`../../assets/icons/${file}`);
        element.html(svg);
        element.addClass('icon');
      }
    };
  })
})(window.angular);