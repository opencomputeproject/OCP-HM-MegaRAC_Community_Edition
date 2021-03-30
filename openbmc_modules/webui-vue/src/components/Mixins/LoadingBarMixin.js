const LoadingBarMixin = {
  methods: {
    startLoader() {
      this.$root.$emit('loader::start');
    },
    endLoader() {
      this.$root.$emit('loader::end');
    },
    hideLoader() {
      this.$root.$emit('loader::hide');
    }
  }
};

export default LoadingBarMixin;
