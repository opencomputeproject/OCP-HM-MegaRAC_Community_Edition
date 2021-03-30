<template>
  <div>
    <header id="page-header">
      <a role="link" class="link-skip-nav btn btn-light" href="#main-content">
        {{ $t('appHeader.skipToContent') }}
      </a>

      <b-navbar
        variant="dark"
        type="dark"
        :aria-label="$t('appHeader.applicationHeader')"
      >
        <!-- Left aligned nav items -->
        <b-button
          id="app-header-trigger"
          class="nav-trigger"
          aria-hidden="true"
          title="Open navigation"
          type="button"
          variant="link"
          :class="{ open: isNavigationOpen }"
          @click="toggleNavigation"
        >
          <icon-close v-if="isNavigationOpen" />
          <icon-menu v-if="!isNavigationOpen" />
        </b-button>
        <b-navbar-nav>
          <img
            class="header-logo"
            src="@/assets/images/logo-header.svg"
            :alt="altLogo"
          />
        </b-navbar-nav>
        <!-- Right aligned nav items -->
        <b-navbar-nav class="ml-auto helper-menu">
          <b-nav-item
            to="/health/event-logs"
            data-test-id="appHeader-container-health"
          >
            <status-icon :status="healthStatusIcon" />
            {{ $t('appHeader.health') }}
          </b-nav-item>
          <b-nav-item
            to="/control/server-power-operations"
            data-test-id="appHeader-container-power"
          >
            <status-icon :status="hostStatusIcon" />
            {{ $t('appHeader.power') }}
          </b-nav-item>
          <!-- Using LI elements instead of b-nav-item to support semantic button elements -->
          <li class="nav-item">
            <b-button
              id="app-header-refresh"
              variant="link"
              data-test-id="appHeader-button-refresh"
              @click="refresh"
            >
              <icon-renew />
              <span class="responsive-text">{{ $t('appHeader.refresh') }}</span>
            </b-button>
          </li>
          <li class="nav-item">
            <b-dropdown
              id="app-header-user"
              variant="link"
              right
              data-test-id="appHeader-container-user"
            >
              <template v-slot:button-content>
                <icon-avatar />
                <span class="responsive-text">{{ username }}</span>
              </template>
              <b-dropdown-item
                to="/profile-settings"
                data-test-id="appHeader-link-profile"
                >{{ $t('appHeader.profileSettings') }}
              </b-dropdown-item>
              <b-dropdown-item
                data-test-id="appHeader-link-logout"
                @click="logout"
              >
                {{ $t('appHeader.logOut') }}
              </b-dropdown-item>
            </b-dropdown>
          </li>
        </b-navbar-nav>
      </b-navbar>
    </header>
    <loading-bar />
  </div>
</template>

<script>
import IconAvatar from '@carbon/icons-vue/es/user--avatar/20';
import IconClose from '@carbon/icons-vue/es/close/20';
import IconMenu from '@carbon/icons-vue/es/menu/20';
import IconRenew from '@carbon/icons-vue/es/renew/20';
import StatusIcon from '../Global/StatusIcon';
import LoadingBar from '../Global/LoadingBar';

export default {
  name: 'AppHeader',
  components: {
    IconAvatar,
    IconClose,
    IconMenu,
    IconRenew,
    StatusIcon,
    LoadingBar
  },
  data() {
    return {
      isNavigationOpen: false,
      altLogo: `${process.env.VUE_APP_COMPANY_NAME} logo`
    };
  },
  computed: {
    hostStatus() {
      return this.$store.getters['global/hostStatus'];
    },
    healthStatus() {
      return this.$store.getters['eventLog/healthStatus'];
    },
    hostStatusIcon() {
      switch (this.hostStatus) {
        case 'on':
          return 'success';
        case 'error':
          return 'danger';
        case 'diagnosticMode':
          return 'warning';
        case 'off':
        default:
          return 'secondary';
      }
    },
    healthStatusIcon() {
      switch (this.healthStatus) {
        case 'OK':
          return 'success';
        case 'Warning':
          return 'warning';
        case 'Critical':
          return 'danger';
        default:
          return 'secondary';
      }
    },
    username() {
      return this.$store.getters['global/username'];
    }
  },
  created() {
    this.getHostInfo();
    this.getEvents();
  },
  mounted() {
    this.$root.$on(
      'change:isNavigationOpen',
      isNavigationOpen => (this.isNavigationOpen = isNavigationOpen)
    );
  },
  methods: {
    getHostInfo() {
      this.$store.dispatch('global/getHostStatus');
    },
    getEvents() {
      this.$store.dispatch('eventLog/getEventLogData');
    },
    refresh() {
      this.$emit('refresh');
    },
    logout() {
      this.$store.dispatch('authentication/logout');
    },
    toggleNavigation() {
      this.$root.$emit('toggle:navigation');
    }
  }
};
</script>

<style lang="scss">
.app-header {
  .link-skip-nav {
    position: absolute;
    top: -60px;
    left: 0.5rem;
    z-index: $zindex-popover;
    transition: $duration--moderate-01 $exit-easing--expressive;
    &:focus {
      top: 0.5rem;
      transition-timing-function: $entrance-easing--expressive;
    }
  }
  .navbar-dark {
    .navbar-text,
    .nav-link,
    .btn-link {
      color: $white !important;
      fill: currentColor;
    }
  }

  .nav-item {
    fill: theme-color('light');
  }

  .navbar {
    padding: 0;
    @include media-breakpoint-up($responsive-layout-bp) {
      height: $header-height;
    }

    .btn-link {
      padding: $spacer / 2;
    }

    .header-logo {
      width: auto;
      height: $header-height;
      padding: $spacer/2 0;
    }

    .helper-menu {
      @include media-breakpoint-down(sm) {
        background-color: gray('800');
        width: 100%;
        justify-content: flex-end;

        .nav-link,
        .btn {
          padding: $spacer / 1.125 $spacer / 2;
        }
      }

      .responsive-text {
        @include media-breakpoint-down(xs) {
          display: none;
        }
      }
    }
  }

  .navbar-nav {
    padding: 0 $spacer;
  }

  .nav-trigger {
    fill: theme-color('light');
    width: $header-height;
    height: $header-height;
    transition: none;

    svg {
      margin: 0;
    }

    &:hover {
      fill: theme-color('light');
      background-color: theme-color('dark');
    }

    &.open {
      background-color: gray('800');
    }

    @include media-breakpoint-up($responsive-layout-bp) {
      display: none;
    }
  }

  .dropdown {
    .dropdown-menu {
      margin-top: 0;
      @include media-breakpoint-up(md) {
        margin-top: 7px;
      }
    }
  }

  .navbar-expand {
    @include media-breakpoint-down(sm) {
      flex-flow: wrap;
    }
  }
}
</style>
