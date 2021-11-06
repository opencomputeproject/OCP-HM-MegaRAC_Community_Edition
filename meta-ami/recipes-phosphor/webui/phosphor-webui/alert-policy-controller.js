/**
 * Controller for SMTP
 *
 * @module app/configuration
 * @exports alerpolicyController
 * @name alertpolicyController
 */

 window.angular && (function(angular) {
    'use strict';
  
    angular.module('app.configuration').controller('alertpolicyController', [
      '$scope', '$window', 'APIUtils', '$route', '$q', 'toastService',
      function($scope, $window, APIUtils, $route, $q, toastService) {
          $scope.alert_policy_configs = {};
          $scope.loading = true;
          $scope.array15 = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15];
          $scope.array40 = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40];
          // $scope.alert_policy_action_arr = [0,1,2,3,4];

          $scope.on_alert_policy_action_set = {
            'Always send alert to this destination' : 0,
            'If previous successful, skip this and continue (if configured)' : 1,
            'If previous successful, stop alerting further' : 2,
            'If previous successful, switch to another channel (if configured)' : 3,
            'If previous successful, switch to another destination type (if configured)' : 4,
          }

          $scope.alert_policy_action_arr = [
            'Always send alert to this destination',
            'If previous successful, skip this and continue (if configured)',
            'If previous successful, stop alerting further',
            'If previous successful, switch to another channel (if configured)',
            'If previous successful, switch to another destination type (if configured)'
          ];

          $scope.on_alert_policy_action_get = {
            0:'Always send alert to this destination',
            1:'If previous successful, skip this and continue (if configured)',
            2:'If previous successful, stop alerting further',
            3:'If previous successful, switch to another channel (if configured)',
            4:'If previous successful, switch to another destination type (if configured)'
          }

          $scope.alert_policy_table = [];


          // Always send alert to this destination = 0
          // If previous successful, skip this and continue (if configured) - 1
          // If previous successful, stop alerting further - 2 
          // If previous successful, switch to another channel (if configured) - 3
          // If previous successful, switch to another destination type (if configured) - 4

          var getAlertPolicydata = APIUtils.getAlertPolicyConfigurations().then(
              function(response){
                var response_data = response.data;
                var alert_policy_data = response_data.data;
                $scope.alert_policy_table = [];
                var tmp_num = 1;
                
                for(var key in alert_policy_data){
                  if(key.match(/AlertPolicyTable\/Entry\d+(_\d+)?$/ig)){
                    var entry = key.split('/').pop();
                    var entry_num = entry.split('Entry').pop();
                    var alert_policy_fill_data = {};
                    entry_num = parseInt(entry_num);

                    if(entry_num > 15 && entry_num <= 30){
                      tmp_num = entry_num - 15;
                    }else if(entry_num > 30 && entry_num <= 45 ){
                      tmp_num = entry_num - 30;
                    }else if(entry_num > 45 && entry_num <= 60 ){
                      tmp_num = entry_num - 45;
                    }else if(entry_num <= 15){
                      tmp_num = entry_num;
                    }

                    alert_policy_fill_data['entry'] = entry_num;
                    alert_policy_fill_data['tmp_num'] = tmp_num;
                    alert_policy_fill_data['channel_number'] = get_channel_number_byte(alert_policy_data[key].ChannelDestSel);
                    alert_policy_fill_data['channel_destination'] = get_channel_dest_byte(alert_policy_data[key].ChannelDestSel);
                    alert_policy_fill_data['alert_string'] = get_alert_string_byte(alert_policy_data[key].AlertStingkey);
                    alert_policy_fill_data['alert_string_key'] = get_alert_string_key_byte(alert_policy_data[key].AlertStingkey);
                    alert_policy_fill_data['enable_filter'] = get_enable_filter_byte(alert_policy_data[key].AlertNum);
                    alert_policy_fill_data['alert_policy_number'] = get_alert_policy_number_byte(alert_policy_data[key].AlertNum);
                    var tmp_alert_policy_action_data = get_alert_policy_action_byte(alert_policy_data[key].AlertNum);
                    alert_policy_fill_data['alert_policy_action'] = $scope.on_alert_policy_action_get[tmp_alert_policy_action_data];

                    $scope.alert_policy_table.push(alert_policy_fill_data);
                  }
                }
                console.log('get alert policy', $scope.alert_policy_table);
              },
              function(error){
                console.log('error', error);
              }
          );

          function get_channel_number_byte(val){
            return (val >> 4) & 0x0f;
          }
          function get_channel_dest_byte(val){
            return val & 0x0f;
          }
          function get_alert_string_byte(val){
            return ((val >> 7) & 0x01);
          }
          function get_alert_string_key_byte(val){
            return (val & 0x7f);
          }
          function get_enable_filter_byte(val){
            return (((val & 0x0f) >> 3) & 0x01);
          }
          function get_alert_policy_number_byte(val){
            return ((val >> 4) & 0x0f);
          }
          function get_alert_policy_action_byte(val){
            return (val & 0x03);
          }

          getAlertPolicydata.finally(function() {
            $scope.loading = false;
          });

          $scope.saveAlertPolicydata = function(enable_filter, alert_policy_number, alert_policy_action, entry){
            console.log("saveAlertPolicydata",enable_filter, alert_policy_number, alert_policy_action, entry);
            var tmp_alert_policy_action = $scope.on_alert_policy_action_set[alert_policy_action];
            var AlertNum = ((alert_policy_number << 4) & 0xf0) | ((enable_filter & 0x01 ) << 3) | (tmp_alert_policy_action & 0x0f);
            if(AlertNum){
              setAlertNumber(AlertNum, entry);
            }
          };

          $scope.savechannelData = function(channel_number, channel_destination, entry){
            console.log("savechannelData",channel_number, channel_destination, entry);
            var d = channel_destination & 0x0f;
            var a = channel_number;
            var c = (a << 4) & 0xf0;
            var ChannelDestSel  = c | d;
            if(ChannelDestSel){
              setChannelDestinationSelector(ChannelDestSel, entry);
            }
          };

          $scope.saveAlertStringData = function(alert_string_key, alert_string, entry){
            console.log("saveAlertStringData",alert_string_key, alert_string, entry);
            var AlertStingkey = ((alert_string << 7) & 0x80) |  (alert_string_key  & 0x7f );
            if(AlertStingkey){
              setAlertStringKey(AlertStingkey, entry);
            }
          };

          function setAlertNumber(AlertNum, entry){
              $scope.loading = true;
              APIUtils.setAlertNumber(AlertNum, entry).then(
                function(response){
                  toastService.success('Alert Policy saved successfully.');
                  $scope.loading = false;
                },
                function(error){
                  console.log(JSON.stringify(error));
                  $scope.loading = false;
              });
          }

          function setAlertStringKey(AlertStingkey, entry){
            $scope.loading = true;
            APIUtils.setAlertStringKey(AlertStingkey, entry).then(
              function(response){
                $scope.loading = false;
              },
              function(error){
                console.log(JSON.stringify(error));
                $scope.loading = false;
            });
          }

          function setChannelDestinationSelector(ChannelDestSel, entry){
            $scope.loading = true;
            APIUtils.setChannelDestinationSelector(ChannelDestSel, entry).then(
              function(response){},
              function(error){
                console.log(JSON.stringify(error));
                $scope.loading = false;
            });
          }
      }
    ]);
  })(angular);
  