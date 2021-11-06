/**
 * Controller for SMTP
 *
 * @module app/configuration
 * @exports smtpController
 * @name smtpController
 */

 window.angular && (function(angular) {
    'use strict';
  
    angular.module('app.configuration').controller('smtpController', [
      '$scope', 'APIUtils', '$route', 'toastService', 'dataService',
      function($scope, APIUtils, $route, toastService, dataService) {
          $scope.smtp_configs = {};
          $scope.loading = true;

          var getSMTPdata = APIUtils.getSMTPConfigurations().then(
              function(response){
                console.log('get SMTP', response.data);
                var SMTPdata = response.data;
                if(SMTPdata.data[0]){
                    $scope.smtp_configs.smtp_enable = true;
                }else{
                    $scope.smtp_configs.smtp_enable = false;
                }
                $scope.smtp_configs.server_address = SMTPdata.data[1];
                $scope.smtp_configs.port = SMTPdata.data[2];
                $scope.smtp_configs.sender_email = SMTPdata.data[3];
                $scope.smtp_configs.reciver_email = SMTPdata.data[4];

                console.log("smtp config", $scope.smtp_configs);
              },
              function(error){
                console.log('error', error);
              }
          );
          getSMTPdata.finally(function() {
            $scope.loading = false;
          });

          $scope.reset = function(){
            $route.reload();
          }

          $scope.func_smtp_enable = function(){
              $scope.smtp_configs.smtp_enable = !$scope.smtp_configs.smtp_enable;
          }

          $scope.saveSMTPSettings = function(){
              console.log("save smtp data", $scope.smtp_configs);
              $scope.loading = true;

              var data = [];
              data.push($scope.smtp_configs.smtp_enable);
              data.push($scope.smtp_configs.server_address);
              data.push($scope.smtp_configs.port);
              data.push($scope.smtp_configs.sender_email);
              data.push($scope.smtp_configs.reciver_email);

              console.log('data to send', data);

              APIUtils.setSMTPConfigurations(data).then(
                  function(response){
                    toastService.success('SMTP settings have been saved.');
                    $scope.loading = false;
                    $scope.reset();
                  },
                  function(error){
                    $scope.loading = false;
                    toastService.error('Error in setting SMTP configurations.');
                    console.log('error', error);
                  }
              )
          }
      }
    ]);
  })(angular);
  