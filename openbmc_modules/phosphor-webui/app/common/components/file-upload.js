window.angular && (function(angular) {
  'use strict';

  const template = `<div class="file-upload">
                      <label
                        for="uploadFile"
                        class="file-upload-btn btn btn-secondary"
                        ng-if="!$ctrl.file"
                        tabindex="0">
                        Choose file
                      </label>
                      <input
                        id="uploadFile"
                        type="file"
                        file=$ctrl.file
                        class="file-upload-input"
                        accept="{{$ctrl.fileType}}"/>
                      <div class="file-upload-container"
                        ng-class="{
                        'file-upload-error' : $ctrl.fileStatus ==='error'}"
                        ng-if="$ctrl.file">
                        <span class="file-filename">
                          {{ $ctrl.file.name }}</span>
                        <status-icon
                          class="file-upload-status"
                          status="{{$ctrl.fileStatus}}">
                        </status-icon>
                        <button
                          type="reset"
                          class="btn file-upload-reset"
                          ng-if="$ctrl.file.name || file"
                          ng-click="$ctrl.file = '';"
                          aria-label="remove selected file">
                          <icon file="icon-close.svg" aria-hidden="true"></icon>
                        </button>
                      </div>
                      <div class="file-upload-btn">
                        <button
                          type="submit"
                          class="btn btn-primary"
                          ng-click="$ctrl.onUpload(); $ctrl.file = '';"
                          ng-if="$ctrl.file"
                          aria-label="upload selected file">
                          Upload
                        </button>
                      </div>
                    </div>`

  const controller = function() {
    this.onUpload = () => {
      this.uploadFile({file: this.file});
    };
  };

  angular.module('app.common.components').component('fileUpload', {
    template,
    controller,
    bindings: {uploadFile: '&', fileType: '@', fileStatus: '@'}
  })
})(window.angular);