window.angular && (function(angular) {
  'use strict';

  angular.module('app.common.directives').directive('remoteLoggingServer', [
    'APIUtils',
    function(APIUtils) {
      return {
        'restrict': 'E', 'template': require('./remote-logging-server.html'),
            'controller': [
              '$scope', '$uibModal', 'toastService',
              function($scope, $uibModal, toastService) {
                const modalActions = {
                  ADD: 0,
                  EDIT: 1,
                  REMOVE: 2,
                  properties: {
                    0: {
                      title: 'Add remote logging server',
                      actionLabel: 'Add',
                      successMessage: 'Connected to remote logging server.',
                      errorMessage: 'Unable to connect to server.'
                    },
                    1: {
                      title: 'Edit remote logging server',
                      actionLabel: 'Save',
                      successMessage: 'Connected to remote logging server.',
                      errorMessage: 'Unable to save remote logging server.'
                    },
                    2: {
                      title: 'Remove remote logging server',
                      actionLabel: 'Remove',
                      successMessage: 'Remote logging server removed.',
                      errorMessage: 'Unable to remove remote logging server.'
                    }
                  }
                };

                const modalTemplate =
                    require('./remote-logging-server-modal.html');

                $scope.activeModal;
                $scope.activeModalProps;

                $scope.remoteServer;
                $scope.remoteServerForm;
                $scope.loadError = true;

                $scope.initModal = (type) => {
                  if (type === undefined) {
                    return;
                  }
                  $scope.activeModal = type;
                  $scope.activeModalProps = modalActions.properties[type];

                  $uibModal
                      .open({
                        template: modalTemplate,
                        windowTopClass: 'uib-modal',
                        scope: $scope,
                        ariaLabelledBy: 'dialog_label'
                      })
                      .result
                      .then((action) => {
                        switch (action) {
                          case modalActions.ADD:
                            addServer();
                            break;
                          case modalActions.EDIT:
                            editServer();
                            break;
                          case modalActions.REMOVE:
                            removeServer();
                            break;
                          default:
                            setFormValues();
                        }
                      })
                      .catch(() => {
                        // reset form when modal overlay clicked
                        // and modal closes
                        setFormValues();
                      })
                };

                const addServer = () => {
                  $scope.loading = true;
                  APIUtils.setRemoteLoggingServer($scope.remoteServerForm)
                      .then(() => {
                        $scope.loading = false;
                        $scope.remoteServer = {...$scope.remoteServerForm};
                        toastService.success(
                            $scope.activeModalProps.successMessage);
                      })
                      .catch(() => {
                        $scope.loading = false;
                        $scope.remoteServer = undefined;
                        setFormValues();
                        toastService.error(
                            $scope.activeModalProps.errorMessage);
                      })
                };

                const editServer = () => {
                  $scope.loading = true;
                  APIUtils.updateRemoteLoggingServer($scope.remoteServerForm)
                      .then(() => {
                        $scope.loading = false;
                        $scope.remoteServer = {...$scope.remoteServerForm};
                        toastService.success(
                            $scope.activeModalProps.successMessage);
                      })
                      .catch(() => {
                        $scope.loading = false;
                        setFormValues();
                        toastService.error(
                            $scope.activeModalProps.errorMessage);
                      })
                };

                const removeServer = () => {
                  $scope.loading = true;
                  APIUtils.disableRemoteLoggingServer()
                      .then(() => {
                        $scope.loading = false;
                        $scope.remoteServer = undefined;
                        setFormValues();
                        toastService.success(
                            $scope.activeModalProps.successMessage);
                      })
                      .catch(() => {
                        $scope.loading = false;
                        toastService.error(
                            $scope.activeModalProps.errorMessage);
                      })
                };

                const setFormValues = () => {
                  $scope.remoteServerForm = {...$scope.remoteServer};
                };

                this.$onInit = () => {
                  APIUtils.getRemoteLoggingServer()
                      .then((remoteServer) => {
                        $scope.loadError = false;
                        $scope.remoteServer = remoteServer;
                        setFormValues();
                      })
                      .catch(() => {
                        $scope.loadError = true;
                      })
                };
              }
            ]
      }
    }
  ])
})(window.angular);