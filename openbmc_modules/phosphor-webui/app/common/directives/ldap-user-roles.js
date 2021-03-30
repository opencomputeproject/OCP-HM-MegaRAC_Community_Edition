window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('ldapUserRoles', [
    'APIUtils',
    function(APIUtils) {
      return {
        restrict: 'E',
        template: require('./ldap-user-roles.html'),
        scope: {roleGroups: '=', enabled: '=', roleGroupType: '='},
        controller: [
          '$scope', 'APIUtils', 'toastService', '$q',
          function($scope, APIUtils, toastService, $q) {
            $scope.privileges = [];
            $scope.loading = true;
            $scope.newGroup = {};
            $scope.selectedGroupIndex = '';
            $scope.editGroup = false;
            $scope.removeGroup = false;
            $scope.removeMultipleGroups = false;
            $scope.all = false;
            $scope.sortPropertyName = 'id';
            $scope.reverse = false;
            $scope.addGroup = false;
            $scope.hasSelectedGroup = false;

            APIUtils.getAccountServiceRoles()
                .then(
                    (data) => {
                      $scope.privileges = data;
                    },
                    (error) => {
                      console.log(JSON.stringify(error));
                    })
                .finally(() => {
                  $scope.loading = false;
                });

            $scope.addGroupFn = () => {
              $scope.addGroup = true;
            };

            $scope.addRoleGroup = () => {
              $scope.loading = true;

              const newGroup = {};
              newGroup.RemoteGroup = $scope.newGroup.RemoteGroup;
              newGroup.LocalRole = $scope.newGroup.LocalRole;

              const data = {};
              const roleGroups = $scope.roleGroups;
              const roleGroupType = $scope.roleGroupType;
              data[roleGroupType] = {};
              data[roleGroupType]['RemoteRoleMapping'] = roleGroups;
              data[roleGroupType]['RemoteRoleMapping'][roleGroups.length] =
                  newGroup;

              APIUtils.saveLdapProperties(data)
                  .then(
                      (response) => {
                        toastService.success(
                            'Group has been created successfully.');
                      },
                      (error) => {
                        toastService.error('Failed to create new group.');
                      })
                  .finally(() => {
                    $scope.loading = false;
                  });
            };

            $scope.editGroupFn = (group, role, index) => {
              $scope.editGroup = true;
              $scope.selectedGroupIndex = index;
              $scope.newGroup.RemoteGroup = group;
              $scope.newGroup.LocalRole = role;
            };

            $scope.editRoleGroup = () => {
              $scope.loading = true;
              const data = {};
              const roleGroupType = $scope.roleGroupType;
              const roleGroups = $scope.roleGroups;
              const localRole = $scope.newGroup.LocalRole;
              const selectedIndex = $scope.selectedGroupIndex;
              data[roleGroupType] = {};
              data[roleGroupType]['RemoteRoleMapping'] = roleGroups;
              data[roleGroupType]['RemoteRoleMapping'][selectedIndex]['LocalRole'] =
                  localRole;

              APIUtils.saveLdapProperties(data)
                  .then(
                      (response) => {
                        toastService.success(
                            'Group has been edited successfully.');
                      },
                      (error) => {
                        toastService.error('Failed to edit group.');
                      })
                  .finally(() => {
                    $scope.loading = false;
                  });
              $scope.editGroup = false;
            };

            $scope.removeGroupFn = index => {
              $scope.removeGroup = true;
              $scope.selectedGroupIndex = index;
            };

            $scope.removeRoleGroup = () => {
              $scope.loading = true;
              const roleGroupType = $scope.roleGroupType;
              const roleGroups = $scope.roleGroups;
              const selectedGroupIndex = $scope.selectedGroupIndex;
              const data = {};
              data[roleGroupType] = {};
              data[roleGroupType]['RemoteRoleMapping'] = roleGroups;
              data[roleGroupType]['RemoteRoleMapping'][selectedGroupIndex] =
                  null;

              APIUtils.saveLdapProperties(data)
                  .then(
                      (response) => {
                        toastService.success(
                            'Group has been removed successfully.');
                      },
                      (error) => {
                        toastService.error('Failed to remove group.');
                      })
                  .finally(() => {
                    $scope.loading = false;
                    $scope.$parent.loadLdap();
                  });
              $scope.removeGroup = false;
            };

            $scope.removeMultipleRoleGroupsFn = () => {
              $scope.removeMultipleGroups = true;
            };

            $scope.roleGroupIsSelectedChanged = () => {
              let groupSelected = false;
              $scope.roleGroups.forEach(group => {
                if (group && group['isSelected']) {
                  groupSelected = true;
                }
              });
              $scope.hasSelectedGroup = groupSelected;
            };

            $scope.removeMultipleRoleGroups = () => {
              $scope.loading = true;
              const roleGroupType = $scope.roleGroupType;
              const roleGroups = $scope.roleGroups;
              const data = {};
              data[roleGroupType] = {};
              data[roleGroupType]['RemoteRoleMapping'] =
                  roleGroups.map((group) => {
                    if (group['isSelected']) {
                      return null;
                    } else {
                      return group;
                    }
                  });

              APIUtils.saveLdapProperties(data)
                  .then(
                      (response) => {
                        toastService.success(
                            'Groups have been removed successfully.');
                      },
                      (error) => {
                        toastService.error('Failed to remove groups.');
                      })
                  .finally(() => {
                    $scope.loading = false;
                    $scope.$parent.loadLdap();
                  });
              $scope.removeMultipleGroups = false;
            };

            $scope.toggleAll = () => {
              let toggleStatus = !$scope.all;
              $scope.roleGroups.forEach((group) => {
                group.isSelected = toggleStatus;
              });
            };

            $scope.optionToggled = () => {
              $scope.all = $scope.roleGroups.every((group) => {
                return group.isSelected;
              });
            };

            $scope.sortBy = (propertyName, isReverse) => {
              $scope.reverse = isReverse;
              $scope.sortPropertyName = propertyName;
            };
          }
        ]
      };
    }
  ]);
})(window.angular);
