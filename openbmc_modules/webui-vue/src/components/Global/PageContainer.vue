<template>
  <main id="main-content" class="page-container">
    <slot />
  </main>
</template>

<script>
export default {
  name: 'PageContainer'
};
</script>

<style lang="scss" scoped>
main {
  width: 100%;
  height: 100%;
  padding-top: $spacer * 1.5;
  padding-bottom: $spacer * 3;
  padding-left: $spacer;
  padding-right: $spacer;
  @include media-breakpoint-up($responsive-layout-bp) {
    padding-left: $spacer * 2;
  }
}
</style>
