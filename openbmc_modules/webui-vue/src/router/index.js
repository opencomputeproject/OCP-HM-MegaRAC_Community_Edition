import Vue from 'vue';
import VueRouter from 'vue-router';
import store from '../store/index';
import AppLayout from '../layouts/AppLayout.vue';
import LoginLayout from '@/layouts/LoginLayout';
import ConsoleLayout from '@/layouts/ConsoleLayout.vue';

Vue.use(VueRouter);

// Meta title is translated using i18n in App.vue and PageTitle.Vue
// Example meta: {title: 'appPageTitle.overview'}
const routes = [
  {
    path: '/',
    meta: {
      requiresAuth: true
    },
    component: AppLayout,
    children: [
      {
        path: '',
        name: 'overview',
        component: () => import('@/views/Overview'),
        meta: {
          title: 'appPageTitle.overview'
        }
      },
      {
        path: '/profile-settings',
        name: 'profile-settings',
        component: () => import('@/views/ProfileSettings'),
        meta: {
          title: 'appPageTitle.profileSettings'
        }
      },
      {
        path: '/health/event-logs',
        name: 'event-logs',
        component: () => import('@/views/Health/EventLogs'),
        meta: {
          title: 'appPageTitle.eventLogs'
        }
      },
      {
        path: '/health/hardware-status',
        name: 'hardware-status',
        component: () => import('@/views/Health/HardwareStatus'),
        meta: {
          title: 'appPageTitle.hardwareStatus'
        }
      },
      {
        path: '/health/sensors',
        name: 'sensors',
        component: () => import('@/views/Health/Sensors'),
        meta: {
          title: 'appPageTitle.sensors'
        }
      },
      {
        path: '/access-control/ldap',
        name: 'ldap',
        component: () => import('@/views/AccessControl/Ldap'),
        meta: {
          title: 'appPageTitle.ldap'
        }
      },
      {
        path: '/access-control/local-user-management',
        name: 'local-users',
        component: () => import('@/views/AccessControl/LocalUserManagement'),
        meta: {
          title: 'appPageTitle.localUserManagement'
        }
      },
      {
        path: '/access-control/ssl-certificates',
        name: 'ssl-certificates',
        component: () => import('@/views/AccessControl/SslCertificates'),
        meta: {
          title: 'appPageTitle.sslCertificates'
        }
      },
      {
        path: '/configuration/date-time-settings',
        name: 'date-time-settings',
        component: () => import('@/views/Configuration/DateTimeSettings'),
        meta: {
          title: 'appPageTitle.dateTimeSettings'
        }
      },
      {
        path: '/control/kvm',
        name: 'kvm',
        component: () => import('@/views/Control/Kvm'),
        meta: {
          title: 'appPageTitle.kvm'
        }
      },
      {
        path: '/control/manage-power-usage',
        name: 'manage-power-usage',
        component: () => import('@/views/Control/ManagePowerUsage'),
        meta: {
          title: 'appPageTitle.managePowerUsage'
        }
      },
      {
        path: '/configuration/network-settings',
        name: 'network-settings',
        component: () => import('@/views/Configuration/NetworkSettings'),
        meta: {
          title: 'appPageTitle.networkSettings'
        }
      },
      {
        path: '/control/reboot-bmc',
        name: 'reboot-bmc',
        component: () => import('@/views/Control/RebootBmc'),
        meta: {
          title: 'appPageTitle.rebootBmc'
        }
      },
      {
        path: '/control/server-led',
        name: 'server-led',
        component: () => import('@/views/Control/ServerLed'),
        meta: {
          title: 'appPageTitle.serverLed'
        }
      },
      {
        path: '/control/serial-over-lan',
        name: 'serial-over-lan',
        component: () => import('@/views/Control/SerialOverLan'),
        meta: {
          title: 'appPageTitle.serialOverLan'
        }
      },
      {
        path: '/control/server-power-operations',
        name: 'server-power-operations',
        component: () => import('@/views/Control/ServerPowerOperations'),
        meta: {
          title: 'appPageTitle.serverPowerOperations'
        }
      },
      {
        path: '/unauthorized',
        name: 'unauthorized',
        component: () => import('@/views/Unauthorized'),
        meta: {
          title: 'appPageTitle.unauthorized'
        }
      }
    ]
  },
  {
    path: '/login',
    component: LoginLayout,
    children: [
      {
        path: '',
        name: 'login',
        component: () => import('@/views/Login'),
        meta: {
          title: 'appPageTitle.login'
        }
      },
      {
        path: '/change-password',
        name: 'change-password',
        component: () => import('@/views/ChangePassword'),
        meta: {
          title: 'appPageTitle.changePassword',
          requiresAuth: true
        }
      }
    ]
  },
  {
    path: '/console',
    component: ConsoleLayout,
    meta: {
      requiresAuth: true
    },
    children: [
      {
        path: 'serial-over-lan-console',
        name: 'serial-over-lan-console',
        component: () =>
          import('@/views/Control/SerialOverLan/SerialOverLanConsole'),
        meta: {
          title: 'appPageTitle.serialOverLan'
        }
      },
      {
        path: 'kvm',
        name: 'kvm-console',
        component: () => import('@/views/Control/Kvm/KvmConsole'),
        meta: {
          title: 'appPageTitle.kvm'
        }
      }
    ]
  }
];

const router = new VueRouter({
  base: process.env.BASE_URL,
  routes,
  linkExactActiveClass: 'nav-link--current'
});

router.beforeEach((to, from, next) => {
  if (to.matched.some(record => record.meta.requiresAuth)) {
    if (store.getters['authentication/isLoggedIn']) {
      next();
      return;
    }
    next('/login');
  } else {
    next();
  }
});

export default router;
