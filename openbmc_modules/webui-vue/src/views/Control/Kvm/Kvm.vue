<template>
  <b-container fluid="xl">
    <page-title />

    <page-section :section-title="$t('pageKvm.subTitle')">
      <div>
        <b-button
          variant="link"
          type="button"
          class="button-launch"
          @click="openConsoleWindow()"
        >
          <icon-launch />
          {{ $t('pageKvm.openNewTab') }}
        </b-button>
      </div>
      <div class="terminal-container">
        <kvm-console />
      </div>
    </page-section>
  </b-container>
</template>

<script>
import IconLaunch from '@carbon/icons-vue/es/launch/32';
import PageTitle from '@/components/Global/PageTitle';
import PageSection from '@/components/Global/PageSection';
import KvmConsole from './KvmConsole';

export default {
  name: 'Kvm',
  components: { IconLaunch, PageSection, PageTitle, KvmConsole },
  methods: {
    openConsoleWindow() {
      window.open(
        '#/console/kvm',
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
.button-launch {
  padding-left: 0px;
}

.terminal-container {
  width: 100%;
}
</style>
