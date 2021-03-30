<template>
  <div>
    <span class="kvm-status">{{ $t('pageKvm.status') }}: {{ status }}</span>
    <b-button
      v-if="isConnected"
      variant="link"
      type="button"
      class="button-launch button-ctrl-alt-delete"
      @click="sendCtrlAltDel"
    >
      {{ $t('pageKvm.buttonCtrlAltDelete') }}
    </b-button>
    <div v-show="isConnected" id="terminal" ref="panel"></div>
  </div>
</template>

<script>
import RFB from '@novnc/novnc/core/rfb';

export default {
  name: 'KvmConsole',
  data() {
    return {
      rfb: null,
      isConnected: false,
      status: this.$t('pageKvm.connecting')
    };
  },
  mounted() {
    this.openTerminal();
  },
  methods: {
    sendCtrlAltDel() {
      this.rfb.sendCtrlAltDel();
    },
    openTerminal() {
      const token = this.$store.getters['authentication/token'];
      this.rfb = new RFB(
        this.$refs.panel,
        `wss://${window.location.host}/kvm/0`,
        { wsProtocols: [token] }
      );

      this.rfb.scaleViewport = true;
      const that = this;
      this.rfb.addEventListener('connect', () => {
        that.isConnected = true;
        that.status = this.$t('pageKvm.connected');
      });

      this.rfb.addEventListener('disconnect', () => {
        that.status = this.$t('pageKvm.disconnected');
      });
    }
  }
};
</script>

<style scoped lang="scss">
#terminal {
  height: calc(100vh - 42px);
}

.button-ctrl-alt-delete {
  float: right;
}

.kvm-status {
  padding-top: $spacer / 2;
  padding-left: $spacer / 4;
  display: inline-block;
}
</style>
