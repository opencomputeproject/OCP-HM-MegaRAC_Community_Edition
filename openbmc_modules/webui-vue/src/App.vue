<template>
  <div id="app">
    <router-view />
  </div>
</template>

<style lang="scss">
@import '@/assets/styles/_obmc-custom';
</style>

<script>
export default {
  name: 'App',
  watch: {
    $route: function(to) {
      document.title = this.$t(to.meta.title) || 'Page is missing title';
    }
  }
};
</script>
