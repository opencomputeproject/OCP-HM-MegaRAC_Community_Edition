<template>
  <div id="terminal" ref="panel"></div>
</template>

<script>
import { AttachAddon } from 'xterm-addon-attach';
import { FitAddon } from 'xterm-addon-fit';
import { Terminal } from 'xterm';

export default {
  name: 'SerialOverLanConsole',
  mounted() {
    this.openTerminal();
  },
  methods: {
    openTerminal() {
      const token = this.$store.getters['authentication/token'];

      const ws = new WebSocket(`wss://${window.location.host}/console0`, [
        token
      ]);

      // Refer https://github.com/xtermjs/xterm.js/ for xterm implementation and addons.

      const term = new Terminal({
        fontSize: 15,
        fontFamily:
          'SFMono-Regular, Menlo, Monaco, Consolas, Liberation Mono, Courier New, monospace'
      });

      const attachAddon = new AttachAddon(ws);
      term.loadAddon(attachAddon);

      const fitAddon = new FitAddon();
      term.loadAddon(fitAddon);

      const SOL_THEME = {
        background: '#19273c',
        cursor: 'rgba(83, 146, 255, .5)',
        scrollbar: 'rgba(83, 146, 255, .5)'
      };
      term.setOption('theme', SOL_THEME);

      term.open(this.$refs.panel);
      fitAddon.fit();

      try {
        ws.onopen = function() {
          console.log('websocket console0/ opened');
        };
        ws.onclose = function(event) {
          console.log(
            'websocket console0/ closed. code: ' +
              event.code +
              ' reason: ' +
              event.reason
          );
        };
      } catch (error) {
        console.log(error);
      }
    }
  }
};
</script>

<style scoped>
@import '~xterm/css/xterm.css';

#terminal {
  height: 25em;
  overflow: hidden;
}
</style>
