<template>
  <transition name="fade">
    <b-progress v-if="!isLoadingComplete" height="0.4rem">
      <b-progress-bar
        striped
        animated
        :value="loadingIndicatorValue"
        :aria-label="$t('global.ariaLabel.progressBar')"
      />
    </b-progress>
  </transition>
</template>

<script>
export default {
  data() {
    return {
      loadingIndicatorValue: 0,
      isLoadingComplete: false,
      loadingIntervalId: null,
      timeoutId: null
    };
  },
  created() {
    this.$root.$on('loader::start', () => {
      this.startLoadingInterval();
    });
    this.$root.$on('loader::end', () => {
      this.endLoadingInterval();
    });
    this.$root.$on('loader::hide', () => {
      this.hideLoadingBar();
    });
  },
  methods: {
    startLoadingInterval() {
      this.clearLoadingInterval();
      this.clearTimeout();
      this.loadingIndicatorValue = 0;
      this.isLoadingComplete = false;
      this.loadingIntervalId = setInterval(() => {
        this.loadingIndicatorValue += 1;
        if (this.loadingIndicatorValue > 100) this.clearLoadingInterval();
      }, 100);
    },
    endLoadingInterval() {
      this.clearLoadingInterval();
      this.clearTimeout();
      this.loadingIndicatorValue = 100;
      this.timeoutId = setTimeout(() => {
        // Let animation complete before hiding
        // the loading bar
        this.isLoadingComplete = true;
      }, 1000);
    },
    hideLoadingBar() {
      this.clearLoadingInterval();
      this.clearTimeout();
      this.loadingIndicatorValue = 0;
      this.isLoadingComplete = true;
    },
    clearLoadingInterval() {
      if (this.loadingIntervalId) clearInterval(this.loadingIntervalId);
      this.loadingIntervalId = null;
    },
    clearTimeout() {
      if (this.timeoutId) clearTimeout(this.timeoutId);
      this.timeoutId = null;
    }
  }
};
</script>

<style lang="scss" scoped>
.progress {
  position: absolute;
  left: 0;
  right: 0;
  bottom: -0.4rem;
  opacity: 1;
  transition: opacity $duration--moderate-01 $standard-easing--productive;

  &.fade-enter,
  &.fade-leave-to {
    opacity: 0;
  }
}
</style>
