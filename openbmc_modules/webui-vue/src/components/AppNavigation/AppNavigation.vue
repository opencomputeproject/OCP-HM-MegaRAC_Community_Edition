<template>
  <div>
    <div class="nav-container" :class="{ open: isNavigationOpen }">
      <nav ref="nav" :aria-label="$t('appNavigation.primaryNavigation')">
        <b-nav vertical class="mb-4">
          <b-nav-item to="/" data-test-id="nav-container-overview">
            <icon-overview />
            {{ $t('appNavigation.overview') }}
          </b-nav-item>

          <li class="nav-item">
            <b-button
              v-b-toggle.health-menu
              variant="link"
              data-test-id="nav-button-health"
            >
              <icon-health />
              {{ $t('appNavigation.health') }}
              <icon-expand class="icon-expand" />
            </b-button>
            <b-collapse id="health-menu" tag="ul" class="nav-item__nav">
              <b-nav-item
                to="/health/event-logs"
                data-test-id="nav-container-event-logs"
              >
                {{ $t('appNavigation.eventLogs') }}
              </b-nav-item>
              <b-nav-item
                to="/health/hardware-status"
                data-test-id="nav-container-hardware-status"
              >
                {{ $t('appNavigation.hardwareStatus') }}
              </b-nav-item>
              <b-nav-item
                to="/health/sensors"
                data-test-id="nav-container-sensors"
              >
                {{ $t('appNavigation.sensors') }}
              </b-nav-item>
            </b-collapse>
          </li>

          <li class="nav-item">
            <b-button
              v-b-toggle.control-menu
              variant="link"
              data-test-id="nav-button-control"
            >
              <icon-control />
              {{ $t('appNavigation.control') }}
              <icon-expand class="icon-expand" />
            </b-button>
            <b-collapse id="control-menu" tag="ul" class="nav-item__nav">
              <b-nav-item to="/control/kvm" data-test-id="nav-container-kvm">
                {{ $t('appNavigation.kvm') }}
              </b-nav-item>
              <b-nav-item
                to="/control/manage-power-usage"
                data-test-id="nav-container-managePowerUsage"
              >
                {{ $t('appNavigation.managePowerUsage') }}
              </b-nav-item>
              <b-nav-item
                to="/control/reboot-bmc"
                data-test-id="nav-container-rebootBmc"
              >
                {{ $t('appNavigation.rebootBmc') }}
              </b-nav-item>
              <b-nav-item
                to="/control/serial-over-lan"
                data-test-id="nav-container-sol"
              >
                {{ $t('appNavigation.serialOverLan') }}
              </b-nav-item>
              <b-nav-item
                to="/control/server-led"
                data-test-id="nav-container-serverLed"
              >
                {{ $t('appNavigation.serverLed') }}
              </b-nav-item>
              <b-nav-item
                to="/control/server-power-operations"
                data-test-id="nav-container-serverPowerOperations"
              >
                {{ $t('appNavigation.serverPowerOperations') }}
              </b-nav-item>
            </b-collapse>
          </li>

          <li class="nav-item">
            <b-button
              v-b-toggle.configuration-menu
              variant="link"
              data-test-id="nav-button-configuration"
            >
              <icon-configuration />
              {{ $t('appNavigation.configuration') }}
              <icon-expand class="icon-expand" />
            </b-button>
            <b-collapse id="configuration-menu" tag="ul" class="nav-item__nav">
              <b-nav-item
                to="/configuration/date-time-settings"
                data-test-id="nav-container-dateTimeSettings"
              >
                {{ $t('appNavigation.dateTimeSettings') }}
              </b-nav-item>
              <b-nav-item
                href="javascript:void(0)"
                data-test-id="nav-container-firmware"
              >
                {{ $t('appNavigation.firmware') }}
              </b-nav-item>
              <b-nav-item
                to="/configuration/network-settings"
                data-test-id="nav-container-networkSettings"
              >
                {{ $t('appNavigation.networkSettings') }}
              </b-nav-item>
              <b-nav-item
                href="javascript:void(0)"
                data-test-id="nav-container-snmp"
              >
                {{ $t('appNavigation.snmpSettings') }}
              </b-nav-item>
            </b-collapse>
          </li>

          <li class="nav-item">
            <b-button
              v-b-toggle.access-control-menu
              variant="link"
              data-test-id="nav-button-accessControl"
            >
              <icon-access-control />
              {{ $t('appNavigation.accessControl') }}
              <icon-expand class="icon-expand" />
            </b-button>
            <b-collapse id="access-control-menu" tag="ul" class="nav-item__nav">
              <b-nav-item
                to="/access-control/ldap"
                data-test-id="nav-container-ldap"
              >
                {{ $t('appNavigation.ldap') }}
              </b-nav-item>
              <b-nav-item
                to="/access-control/local-user-management"
                data-test-id="nav-container-localUserManagement"
              >
                {{ $t('appNavigation.localUserManagement') }}
              </b-nav-item>
              <b-nav-item
                to="/access-control/ssl-certificates"
                data-test-id="nav-container-sslCertificates"
              >
                {{ $t('appNavigation.sslCertificates') }}
              </b-nav-item>
            </b-collapse>
          </li>
        </b-nav>
      </nav>
    </div>
    <transition name="fade">
      <div
        v-if="isNavigationOpen"
        id="nav-overlay"
        class="nav-overlay"
        @click="toggleIsOpen"
      ></div>
    </transition>
  </div>
</template>

<script>
import IconAnalytics from '@carbon/icons-vue/es/analytics/16';
import IconDataCheck from '@carbon/icons-vue/es/data--check/16';
import IconSettingsAdjust from '@carbon/icons-vue/es/settings--adjust/16';
import IconSettings from '@carbon/icons-vue/es/settings/16';
import IconPassword from '@carbon/icons-vue/es/password/16';
import IconChevronUp from '@carbon/icons-vue/es/chevron--up/16';

export default {
  name: 'AppNavigation',
  components: {
    iconOverview: IconAnalytics,
    iconHealth: IconDataCheck,
    iconControl: IconSettingsAdjust,
    iconConfiguration: IconSettings,
    iconAccessControl: IconPassword,
    iconExpand: IconChevronUp
  },
  data() {
    return {
      isNavigationOpen: false
    };
  },
  watch: {
    $route: function() {
      this.isNavigationOpen = false;
    },
    isNavigationOpen: function(isNavigationOpen) {
      this.$root.$emit('change:isNavigationOpen', isNavigationOpen);
    }
  },
  mounted() {
    this.$root.$on('toggle:navigation', () => this.toggleIsOpen());
  },
  methods: {
    toggleIsOpen() {
      this.isNavigationOpen = !this.isNavigationOpen;
    }
  }
};
</script>

<style scoped lang="scss">
svg {
  fill: currentColor;
  height: 1.2rem;
  width: 1.2rem;
  margin-left: 0 !important; //!important overriding button specificity
  vertical-align: text-bottom;
  &:not(.icon-expand) {
    margin-right: $spacer;
  }
}

.nav {
  padding-top: $spacer / 4;
  @include media-breakpoint-up($responsive-layout-bp) {
    padding-top: $spacer;
  }
}

.nav-item__nav {
  list-style: none;
  padding-left: 0;
  margin-left: 0;

  .nav-item {
    outline: none;
  }

  .nav-link {
    padding-left: $spacer * 4;
    outline: none;

    &:not(.nav-link--current) {
      font-weight: normal;
    }
  }
}

.btn-link {
  width: 100%;
  text-align: left;
  text-decoration: none !important;
  border-radius: 0;

  &.collapsed {
    .icon-expand {
      transform: rotate(180deg);
    }
  }
}

.icon-expand {
  float: right;
  margin-top: $spacer / 4;
}

.btn-link,
.nav-link {
  position: relative;
  font-weight: $headings-font-weight;
  padding-left: $spacer; // defining consistent padding for links and buttons
  padding-right: $spacer;
  color: theme-color('secondary');

  &:hover {
    background-color: gray('300');
    color: theme-color('dark');
  }

  &:focus {
    box-shadow: $btn-focus-box-shadow;
    color: theme-color('dark');
  }
}

.nav-link--current,
.nav-link--current:hover,
.nav-link--current:focus {
  font-weight: $headings-font-weight;
  background-color: theme-color('secondary');
  color: theme-color('light');
  cursor: default;

  &::before {
    content: '';
    position: absolute;
    top: 0;
    bottom: 0;
    left: 0;
    width: 4px;
    background-color: theme-color('primary');
  }
}

.nav-container {
  position: fixed;
  width: $navigation-width;
  top: $header-height;
  bottom: 0;
  left: 0;
  z-index: $zindex-fixed;
  overflow-y: auto;
  background-color: gray('100');
  transform: translateX(-$navigation-width);
  transition: transform $exit-easing--productive $duration--moderate-02;
  @include media-breakpoint-down(md) {
    z-index: $zindex-fixed + 2;
  }

  &.open,
  &:focus-within {
    transform: translateX(0);
    transition-timing-function: $entrance-easing--productive;
  }

  @include media-breakpoint-up($responsive-layout-bp) {
    transition-duration: $duration--fast-01;
    transform: translateX(0);
  }
}

.nav-overlay {
  position: fixed;
  top: $header-height;
  bottom: 0;
  left: 0;
  right: 0;
  z-index: $zindex-fixed + 1;
  background-color: $black;
  opacity: 0.5;

  &.fade-enter-active {
    transition: opacity $duration--moderate-02 $entrance-easing--productive;
  }

  &.fade-leave-active {
    transition: opacity $duration--fast-02 $exit-easing--productive;
  }

  &.fade-enter,
  &.fade-leave-to {
    opacity: 0;
  }

  @include media-breakpoint-up($responsive-layout-bp) {
    display: none;
  }
}
</style>
