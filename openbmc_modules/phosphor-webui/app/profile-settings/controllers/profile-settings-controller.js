/**
 * Controller for the profile settings page
 *
 * @module app/profile-settings/controllers/index
 * @exports ProfileSettingsController
 * @name ProfileSettingsController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.profileSettings')
      .controller('profileSettingsController', [
        '$scope', 'APIUtils', 'dataService', 'toastService',
        function($scope, APIUtils, dataService, toastService) {
          $scope.username;
          $scope.minPasswordLength;
          $scope.maxPasswordLength;
          $scope.password;
          $scope.passwordConfirm;

          /**
           * Make API call to update user password
           * @param {string} password
           */
          const updatePassword = function(password) {
            $scope.loading = true;
            APIUtils.updateUser($scope.username, null, password)
                .then(
                    () => toastService.success(
                        'Password has been updated successfully.'))
                .catch((error) => {
                  console.log(JSON.stringify(error));
                  toastService.error('Unable to update password.')
                })
                .finally(() => {
                  $scope.password = '';
                  $scope.passwordConfirm = '';
                  $scope.form.$setPristine();
                  $scope.form.$setUntouched();
                  $scope.loading = false;
                })
          };

          /**
           * API call to get account settings for min/max
           * password length requirement
           */
          const getAllUserAccountProperties = function() {
            APIUtils.getAllUserAccountProperties().then((accountSettings) => {
              $scope.minPasswordLength = accountSettings.MinPasswordLength;
              $scope.maxPasswordLength = accountSettings.MaxPasswordLength;
            })
          };

          /**
           * Callback after form submitted
           */
          $scope.onSubmit = function(form) {
            if (form.$valid) {
              const password = form.password.$viewValue;
              updatePassword(password);
            }
          };

          /**
           * Callback after view loaded
           */
          $scope.$on('$viewContentLoaded', () => {
            getAllUserAccountProperties();
            $scope.username = dataService.getUser();
          });
        }
      ]);
})(angular);
