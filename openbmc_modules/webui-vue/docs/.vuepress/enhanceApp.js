
import "./styles/_index.scss";
import Alert from "../../src/components/Global/Alert";
import BVToastMixin from "../../src/components/Mixins/BVToastMixin";

// Bootstrap-vue Plugin imports
import {
    AlertPlugin,
    ButtonPlugin,
    ToastPlugin
  } from 'bootstrap-vue';


export default ({ Vue }) => {
      Vue.use(AlertPlugin);
      Vue.use(ButtonPlugin);
      Vue.use(ToastPlugin);

      // BMC Components and Mixins
      Vue.component('Alert', Alert);
      Vue.mixin('BVToastMixin', BVToastMixin);
}