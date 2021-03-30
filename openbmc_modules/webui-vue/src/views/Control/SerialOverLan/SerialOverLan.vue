<template>
  <b-container fluid="xl">
    <page-title />

    <page-section :section-title="$t('pageSerialoverLAN.subTitle')">
      <p>{{ $t('pageSerialoverLAN.subTitleDesc') }}</p>

      <div class="terminal-container">
        <serial-over-lan-console />
      </div>
      <div class="text-right">
        <b-button
          variant="link"
          type="button"
          class="button-launch"
          @click="openConsoleWindow()"
        >
          <icon-launch />

          {{ $t('pageSerialoverLAN.openNewTab') }}
        </b-button>
      </div>
    </page-section>
  </b-container>
</template>

<script>
import IconLaunch from '@carbon/icons-vue/es/launch/32';
import PageTitle from '@/components/Global/PageTitle';
import PageSection from '@/components/Global/PageSection';
import SerialOverLanConsole from './SerialOverLanConsole';

export default {
  name: 'SerialOverLan',
  components: { IconLaunch, PageSection, PageTitle, SerialOverLanConsole },
  methods: {
    openConsoleWindow() {
      window.open(
        '#/console/serial-over-lan-console',
        '_blank',
        'directories=no,titlebar=no,toolbar=no,location=no,status=no,menubar=no,scrollbars=no,resizable=yes,width=600,height=550'
      );
    }
  }
};
</script>

<style scoped>
.button-launch > svg {
  height: 25px;
}
.terminal-container {
  width: 100%;
}
</style>
