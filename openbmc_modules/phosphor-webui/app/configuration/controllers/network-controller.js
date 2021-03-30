/**
 * Controller for network
 *
 * @module app/configuration
 * @exports networkController
 * @name networkController
 */

window.angular && (function(angular) {
  'use strict';

  angular.module('app.configuration').controller('networkController', [
    '$scope', '$window', 'APIUtils', 'dataService', '$timeout', '$route', '$q',
    'toastService',
    function(
        $scope, $window, APIUtils, dataService, $timeout, $route, $q,
        toastService) {
      $scope.dataService = dataService;
      $scope.network = {};
      $scope.oldInterface = {};
      $scope.interface = {};
      $scope.networkDevice = false;
      $scope.hostname = '';
      $scope.defaultGateway = '';
      $scope.selectedInterface = '';
      $scope.confirmSettings = false;
      $scope.loading = false;
      $scope.ipv4sToDelete = [];

      loadNetworkInfo();

      $scope.selectInterface = function(interfaceId) {
        $scope.interface = $scope.network.interfaces[interfaceId];
        // Copy the interface so we know later if changes were made to the page
        $scope.oldInterface = JSON.parse(JSON.stringify($scope.interface));
        $scope.selectedInterface = interfaceId;
        $scope.networkDevice = false;
      };

      $scope.addDNSField = function() {
        $scope.interface.Nameservers.push('');
      };

      $scope.removeDNSField = function(index) {
        $scope.interface.Nameservers.splice(index, 1);
      };

      $scope.addIpv4Field = function() {
        $scope.interface.ipv4.values.push(
            {Address: '', PrefixLength: '', Gateway: ''});
      };

      $scope.removeIpv4Address = function(index) {
        // Check if the IPV4 being removed has an id. This indicates that it is
        // an existing address and needs to be removed in the back end.
        if ($scope.interface.ipv4.values[index].id) {
          $scope.ipv4sToDelete.push($scope.interface.ipv4.values[index]);
        }
        $scope.interface.ipv4.values.splice(index, 1);
      };

      $scope.setNetworkSettings = function() {
        // Hides the confirm network settings modal
        $scope.confirmSettings = false;
        $scope.loading = true;
        var promises = [];

        // MAC Address are case-insensitive
        if ($scope.interface.MACAddress.toLowerCase() !=
            dataService.mac_address.toLowerCase()) {
          promises.push(setMACAddress());
        }
        if ($scope.defaultGateway != dataService.defaultgateway) {
          promises.push(setDefaultGateway());
        }
        if ($scope.hostname != dataService.hostname) {
          promises.push(setHostname());
        }
        if ($scope.interface.DHCPEnabled != $scope.oldInterface.DHCPEnabled) {
          promises.push(setDHCPEnabled());
        }

        // Remove any empty strings from the array. Important because we add an
        // empty string to the end so the user can add a new DNS server, if the
        // user doesn't fill out the field, we don't want to add.
        $scope.interface.Nameservers =
            $scope.interface.Nameservers.filter(Boolean);
        // toString() is a cheap way to compare 2 string arrays
        if ($scope.interface.Nameservers.toString() !=
            $scope.oldInterface.Nameservers.toString()) {
          promises.push(setNameservers());
        }

        // Set IPV4 IP Addresses, Netmask Prefix Lengths, and Gateways
        if (!$scope.interface.DHCPEnabled) {
          // Delete existing IPV4 addresses that were removed
          promises.push(removeIPV4s());
          // Update any changed IPV4 addresses and add new
          for (var i in $scope.interface.ipv4.values) {
            if (!APIUtils.validIPV4IP(
                    $scope.interface.ipv4.values[i].Address)) {
              toastService.error(
                  $scope.interface.ipv4.values[i].Address +
                  ' invalid IP parameter');
              $scope.loading = false;
              return;
            }
            if ($scope.interface.ipv4.values[i].Gateway &&
                !APIUtils.validIPV4IP(
                    $scope.interface.ipv4.values[i].Gateway)) {
              toastService.error(
                  $scope.interface.ipv4.values[i].Address +
                  ' invalid gateway parameter');
              $scope.loading = false;
              return;
            }
            // The netmask prefix length will be undefined if outside range
            if (!$scope.interface.ipv4.values[i].PrefixLength) {
              toastService.error(
                  $scope.interface.ipv4.values[i].Address +
                  ' invalid Prefix Length parameter');
              $scope.loading = false;
              return;
            }
            if ($scope.interface.ipv4.values[i].updateAddress ||
                $scope.interface.ipv4.values[i].updateGateway ||
                $scope.interface.ipv4.values[i].updatePrefix) {
              // If IPV4 has an id it means it already exists in the back end,
              // and in order to update it is required to remove previous IPV4
              // address and add new one. See openbmc/openbmc/issues/2163.
              // TODO: update to use PUT once issue 2163 is resolved.
              if ($scope.interface.ipv4.values[i].id) {
                promises.push(updateIPV4(i));
              } else {
                promises.push(addIPV4(i));
              }
            }
          }
        }

        if (promises.length) {
          $q.all(promises).then(
              function(response) {
                // Since an IPV4 interface (e.g. IP address, gateway, or
                // netmask) edit is a delete then an add and the GUI can't
                // calculate the interface id (e.g. 5c083707) beforehand and it
                // is not returned by the REST call, openbmc#3227, reload the
                // page after an edit, which makes a 2nd REST call. Do this for
                // all network changes due to the possibility of a set network
                // failing even though it returned success, openbmc#1641, and to
                // update dataService and oldInterface to know which data has
                // changed if the user continues to edit network settings.
                // TODO: The reload is not ideal. Revisit this.
                $timeout(function() {
                  loadNetworkInfo();
                  $scope.loading = false;
                  toastService.success('Network settings saved');
                }, 4000);
              },
              function(error) {
                $scope.loading = false;
                toastService.error('Network settings could not be saved');
              })
        } else {
          $scope.loading = false;
        }
      };

      function setMACAddress() {
        return APIUtils
            .setMACAddress(
                $scope.selectedInterface, $scope.interface.MACAddress)
            .then(
                function(data) {},
                function(error) {
                  console.log(JSON.stringify(error));
                  return $q.reject();
                });
      }

      function setDefaultGateway() {
        return APIUtils.setDefaultGateway($scope.defaultGateway)
            .then(
                function(data) {},
                function(error) {
                  console.log(JSON.stringify(error));
                  return $q.reject();
                });
      }

      function setHostname() {
        return APIUtils.setHostname($scope.hostname)
            .then(
                function(data) {},
                function(error) {
                  console.log(JSON.stringify(error));
                  return $q.reject();
                });
      }

      function setDHCPEnabled() {
        return APIUtils
            .setDHCPEnabled(
                $scope.selectedInterface, $scope.interface.DHCPEnabled)
            .then(
                function(data) {},
                function(error) {
                  console.log(JSON.stringify(error));
                  return $q.reject();
                });
      }

      function setNameservers() {
        return APIUtils
            .setNameservers(
                $scope.selectedInterface, $scope.interface.Nameservers)
            .then(
                function(data) {},
                function(error) {
                  console.log(JSON.stringify(error));
                  return $q.reject();
                });
      }

      function removeIPV4s() {
        return $scope.ipv4sToDelete.map(function(ipv4) {
          return APIUtils.deleteIPV4($scope.selectedInterface, ipv4.id)
              .then(
                  function(data) {},
                  function(error) {
                    console.log(JSON.stringify(error));
                    return $q.reject();
                  })
        });
      }

      function addIPV4(index) {
        return APIUtils
            .addIPV4(
                $scope.selectedInterface,
                $scope.interface.ipv4.values[index].Address,
                $scope.interface.ipv4.values[index].PrefixLength,
                $scope.interface.ipv4.values[index].Gateway)
            .then(
                function(data) {},
                function(error) {
                  console.log(JSON.stringify(error));
                  return $q.reject();
                })
      }

      function updateIPV4(index) {
        // The correct way to edit an IPV4 interface is to remove it and then
        // add a new one
        return APIUtils
            .deleteIPV4(
                $scope.selectedInterface,
                $scope.interface.ipv4.values[index].id)
            .then(
                function(data) {
                  return APIUtils
                      .addIPV4(
                          $scope.selectedInterface,
                          $scope.interface.ipv4.values[index].Address,
                          $scope.interface.ipv4.values[index].PrefixLength,
                          $scope.interface.ipv4.values[index].Gateway)
                      .then(
                          function(data) {},
                          function(error) {
                            console.log(JSON.stringify(error));
                            return $q.reject();
                          });
                },
                function(error) {
                  console.log(JSON.stringify(error));
                  return $q.reject();
                });
      }

      $scope.refresh = function() {
        loadNetworkInfo();
      };

      function loadNetworkInfo() {
        APIUtils.getNetworkInfo().then(function(data) {
          dataService.setNetworkInfo(data);
          $scope.network = data.formatted_data;
          $scope.hostname = data.hostname;
          $scope.defaultGateway = data.defaultgateway;
          if ($scope.network.interface_ids.length) {
            // Use the first network interface if the user hasn't chosen one
            if (!$scope.selectedInterface ||
                !$scope.network.interfaces[$scope.selectedInterface]) {
              $scope.selectedInterface = $scope.network.interface_ids[0];
            }
            $scope.interface =
                $scope.network.interfaces[$scope.selectedInterface];
            // Copy the interface so we know later if changes were made to the
            // page
            $scope.oldInterface = JSON.parse(JSON.stringify($scope.interface));
          }
          // Add id values and update flags to corresponding IPV4 objects
          for (var i = 0; i < $scope.interface.ipv4.values.length; i++) {
            $scope.interface.ipv4.values[i].id = $scope.interface.ipv4.ids[i];
            $scope.interface.ipv4.values[i].updateAddress = false;
            $scope.interface.ipv4.values[i].updateGateway = false;
            $scope.interface.ipv4.values[i].updatePrefix = false;
          }
        });
      }
    }
  ]);
})(angular);
