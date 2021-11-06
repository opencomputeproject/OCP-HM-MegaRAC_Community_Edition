/**
 * Controller for SMTP
 *
 * @module app/configuration
 * @exports eventfilterController
 * @name eventfilterController
 */

 window.angular && (function(angular) {
    'use strict';
  
    angular.module('app.configuration').controller('eventfilterController', [
      '$scope', '$window', 'APIUtils', '$route', '$q', 'toastService',
      function($scope, $window, APIUtils, $route, $q, toastService) {
          $scope.alert_policy_configs = {};
          $scope.loading = true;
          $scope.severity = {
            0 : 'Any severity',
            1: 'New monitor state',
            2 : 'New information',
            4: 'Normal state',
            8: 'Non-Critical state',
            16:  'Critical state',
            32:  'Non-Recoverable state'
          };
          $scope.severity_send = {
            'Any severity': 0,
            'New monitor state': 1,
            'New information': 2,
            'Normal state': 4,
            'Non-Critical state': 8,
            'Critical state': 16,
            'Non-Recoverable state': 32,
          };
          $scope.power_Action_data = {
            0:'None',
            2:'Power Down',
            8:'Power Cycle',
            4:'Reset'
          };
          $scope.power_Action_data_send = {
            'None': 0,
            'Power Down': 2,
            'Power Cycle': 8,
            'Reset' : 4
          };
          $scope.array15 = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15];
          $scope.ev_op = 'All Events';

          var sensor_numbers = APIUtils.getSensorNumbers(function(response){
            $scope.sensor_name_with_ID = response;
          });

          var get_Sensor_data = APIUtils.getAllSensorStatus(function(data, originalData) {
                $scope.sensor_type_arr = ['All Sensors'];
                $scope.sensor_name_arr = ["All Sensors"];
                $scope.all_sensor_name_arr = [];
                $scope.sensor_data_arr = [];
                for(var i in data){
                    var s_obj = {};
                    var sensor_type_name = data[i].path.split('/')[4];
                    s_obj['sensor_type_name'] = sensor_type_name;
                    s_obj['sensor_type'] = data[i].SensorType;
                    s_obj['sensor_name'] = data[i].title;
                    if($scope.sensor_type_arr.indexOf(sensor_type_name) == -1){
                        $scope.sensor_type_arr.push(sensor_type_name);
                    }
                    $scope.sensor_name_arr.push(data[i].title);
                    $scope.sensor_data_arr.push(s_obj);
                }
                $scope.all_sensor_name_arr = JSON.parse(JSON.stringify($scope.sensor_name_arr));
                getEventFilterdata();
          });

          $scope.change_sensor_type = function(sen_type, index){
            console.log('sen_type',sen_type);
              var selected_sensor_type = '';
              // Empty the sensor array
              $scope.event_filter_table[index].custom_sensor_name_array = [];

              if(sen_type == 'All Sensors'){
                $scope.event_filter_table[index].custom_sensor_name_array = $scope.all_sensor_name_arr;
                $scope.event_filter_table[index].SensorType = 255;
                return;
              }

              // update the sensors array with selected sensor type
              for(var i in $scope.sensor_data_arr){
                  if($scope.sensor_data_arr[i].sensor_type_name == sen_type){
                    // selected_sensor_type = $scope.sensor_data_arr[i].sensor_type;
                    $scope.event_filter_table[index].custom_sensor_name_array.push($scope.sensor_data_arr[i].sensor_name);
                  }
              }
              console.log($scope.event_filter_table[index].custom_sensor_name_array);
              console.log($scope.event_filter_table[index]);
              // $scope.event_filter_table[index].SensorType = selected_sensor_type;
          }

          $scope.add_selected_sensor = function(sensor_name, index){
            console.log('sensor_name', sensor_name, index);
            if(sensor_name == 'All Sensors'){
              $scope.event_filter_table[index].SensorNum = 255;
              return;
            }
            for(var i in $scope.sensor_name_with_ID){
              if($scope.sensor_name_with_ID[i].sensor_name == sensor_name){
                console.log($scope.sensor_name_with_ID[i]);
                $scope.event_filter_table[index].SensorNum = $scope.sensor_name_with_ID[i].sensor_ID;
              }
            }
            for(var i in $scope.sensor_data_arr){
              if($scope.sensor_data_arr[i].sensor_name == sensor_name){
                $scope.event_filter_table[index].SensorType = $scope.sensor_data_arr[i].sensor_type;
              }
            }
          }


          $scope.saveEventFilterData = function(data){
            $scope.loading = true;
            var promises = [];
            console.log('data', data);
            var entry_num = data.custom_entry;
            for(var i in data){
              if(i.indexOf('custom_') == -1 && i.indexOf('$$') == -1){
                // console.log(entry_num, i, data[i]);

                if(i == 'EventSeverity'){
                  var att = $scope.severity_send[data.custom_EventSeverity];
                  promises.push(CallsaveEventFilterDataAPI(entry_num, i, att));
                  continue;
                }
                if(i == 'EvtFilterAction'){
                  var att = $scope.power_Action_data_send[data.custom_EvtFilterPowerAction];
                  var data_to_send = data.EvtFilterAction | att;
                  promises.push(CallsaveEventFilterDataAPI(entry_num, i, data_to_send));
                  continue;
                }
                if(i == 'FilterConfig'){
                  var att = (data.FilterConfig << 7);
                  promises.push(CallsaveEventFilterDataAPI(entry_num, i, att));
                  continue;
                }
                promises.push(CallsaveEventFilterDataAPI(entry_num, i, data[i]));
              }
            }

            if (promises.length) {
              $q.all(promises).then(function(){
                toastService.success('Event Filter settings saved');
                $scope.loading = false;
                $route.reload();
              }, function(){
                $scope.loading = false;
                toastService.error('Event Filter Settings could not be saved');
              });
            }
            // console.log('save data', data);
          }

          function CallsaveEventFilterDataAPI(Entry, attr, attr_data){
            console.log("calling", Entry, attr, attr_data);
            return APIUtils.SaveEventFilterData(Entry, attr, attr_data).then(function(){},
            function(error){
              console.log(JSON.stringify(error));
              return $q.reject();
            })
          }

          function get_sensor_name(SensorNum){
            if(SensorNum == 255){ return 'All Sensors'; }

            for(var i in $scope.sensor_name_with_ID){
              if($scope.sensor_name_with_ID[i].sensor_ID == SensorNum){
                return $scope.sensor_name_with_ID[i].sensor_name;
              }
            }
          }

          function get_sensor_type_name(SensorType){
            // console.log('sensor_name_with_ID', $scope.sensor_data_arr);
            if(SensorType == 255){ return 'All Sensors'; }

            for(var i in $scope.sensor_data_arr){
              if($scope.sensor_data_arr[i].sensor_type == SensorType){
                return $scope.sensor_data_arr[i].sensor_type_name;
              }
            }
          }

          function get_event_filter_action(data){
            return data & 0x01;
            // var poweraction = filteraction & 0x0E
          }

          function get_event_filter_power_action(data){
            var poweraction = data & 0x0E;
            return $scope.power_Action_data[poweraction];
          }

          function getFilterConfigdata(data){
            console.log('getFilterConfigdata', data);
            var tmp = (data >> 7);
            tmp = tmp & 0x01;
            console.log('tmp', tmp);
            return tmp;
          }

          function get_event_Severity_data(data){
            return $scope.severity[data];
          }

          function getEventFilterdata() {
            $scope.loading = true;
            APIUtils.getEventFilterConfigurations().then(
              function(response){
                $scope.loading = false;
                var response_data = response.data;
                var event_filter_data = response_data.data;
                $scope.event_filter_table = [];
                // console.log('$scope.sensor_type_arr', $scope.sensor_type_arr);
                // console.log('$scope.sensor_name_arr', $scope.sensor_name_arr);
                var i = 0;
                for(var key in event_filter_data){
                    if(key.match(/EventFilterTable\/Entry\d+(_\d+)?$/ig)){
                        var entry = key.split('/').pop();
                        var entry_num = entry.split('Entry').pop();
                        event_filter_data[key]['custom_entry'] = parseInt(entry_num);
                        event_filter_data[key]['custom_index_for_update'] = i;
                        event_filter_data[key]['custom_sensor_name'] = get_sensor_name(event_filter_data[key].SensorNum);
                        event_filter_data[key]['custom_sensor_type_name'] = get_sensor_type_name(event_filter_data[key].SensorType);
                        event_filter_data[key]['custom_sensor_type_array'] = $scope.sensor_type_arr;
                        event_filter_data[key]['custom_sensor_name_array'] = $scope.sensor_name_arr;
                        event_filter_data[key]['custom_EvtFilterPowerAction'] = get_event_filter_power_action(event_filter_data[key].EvtFilterAction);
                        event_filter_data[key]['custom_EventSeverity'] = get_event_Severity_data(event_filter_data[key].EventSeverity);
                        event_filter_data[key].EvtFilterAction = get_event_filter_action(event_filter_data[key].EvtFilterAction);
                        event_filter_data[key].FilterConfig = getFilterConfigdata(event_filter_data[key].FilterConfig);
                        event_filter_data[key].EventData1OffsetMask = 65535;

                        $scope.event_filter_table.push(event_filter_data[key]);
                    }
                  i++;
                }
                console.log('get event filter', $scope.event_filter_table);
              },
              function(error){
                $scope.loading = false;
                console.log('error', error);
              }
            );
          };
      }
    ]);
  })(angular);
  
