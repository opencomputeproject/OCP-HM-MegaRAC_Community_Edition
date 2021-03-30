import Vue from 'vue';
import App from './App.vue';
import router from './router';
import store from './store';
import {
  AlertPlugin,
  BadgePlugin,
  ButtonPlugin,
  BVConfigPlugin,
  CollapsePlugin,
  DropdownPlugin,
  FormPlugin,
  FormCheckboxPlugin,
  FormDatepickerPlugin,
  FormFilePlugin,
  FormGroupPlugin,
  FormInputPlugin,
  FormRadioPlugin,
  FormSelectPlugin,
  FormTagsPlugin,
  InputGroupPlugin,
  LayoutPlugin,
  LinkPlugin,
  ListGroupPlugin,
  ModalPlugin,
  NavbarPlugin,
  NavPlugin,
  PaginationPlugin,
  ProgressPlugin,
  TablePlugin,
  ToastPlugin,
  TooltipPlugin
} from 'bootstrap-vue';
import Vuelidate from 'vuelidate';
import i18n from './i18n';
import { format } from 'date-fns-tz';

// Filters
Vue.filter('shortTimeZone', function(value) {
  const longTZ = value
    .toString()
    .match(/\((.*)\)/)
    .pop();
  const regexNotUpper = /[*a-z ]/g;
  return longTZ.replace(regexNotUpper, '');
});

Vue.filter('formatDate', function(value) {
  const isUtcDisplay = store.getters['global/isUtcDisplay'];

  if (value instanceof Date) {
    if (isUtcDisplay) {
      return value.toISOString().substring(0, 10);
    }
    const pattern = `yyyy-MM-dd`;
    const timezone = Intl.DateTimeFormat().resolvedOptions().timeZone;
    return format(value, pattern, { timezone });
  }
});

Vue.filter('formatTime', function(value) {
  const isUtcDisplay = store.getters['global/isUtcDisplay'];

  if (value instanceof Date) {
    if (isUtcDisplay) {
      let timeOptions = {
        timeZone: 'UTC',
        hour12: false
      };
      return `${value.toLocaleTimeString('default', timeOptions)} UTC`;
    }
    const timezone = Intl.DateTimeFormat().resolvedOptions().timeZone;
    const shortTz = Vue.filter('shortTimeZone')(value);
    const pattern = `HH:mm:ss ('${shortTz}' O)`;
    return format(value, pattern, { timezone }).replace('GMT', 'UTC');
  }
});

// Plugins
Vue.use(AlertPlugin);
Vue.use(BadgePlugin);
Vue.use(ButtonPlugin);
Vue.use(BVConfigPlugin, {
  BFormText: { textVariant: 'secondary' },
  BTable: {
    headVariant: 'light',
    footVariant: 'light'
  },
  BFormTags: {
    tagVariant: 'primary',
    addButtonVariant: 'link-primary'
  },
  BBadge: {
    variant: 'primary'
  }
});
Vue.use(CollapsePlugin);
Vue.use(DropdownPlugin);
Vue.use(FormPlugin);
Vue.use(FormCheckboxPlugin);
Vue.use(FormDatepickerPlugin);
Vue.use(FormFilePlugin);
Vue.use(FormGroupPlugin);
Vue.use(FormInputPlugin);
Vue.use(FormRadioPlugin);
Vue.use(FormSelectPlugin);
Vue.use(FormTagsPlugin);
Vue.use(InputGroupPlugin);
Vue.use(LayoutPlugin);
Vue.use(LayoutPlugin);
Vue.use(LinkPlugin);
Vue.use(ListGroupPlugin);
Vue.use(ModalPlugin);
Vue.use(NavbarPlugin);
Vue.use(NavPlugin);
Vue.use(PaginationPlugin);
Vue.use(ProgressPlugin);
Vue.use(TablePlugin);
Vue.use(ToastPlugin);
Vue.use(TooltipPlugin);
Vue.use(Vuelidate);

new Vue({
  router,
  store,
  i18n,
  render: h => h(App)
}).$mount('#app');
