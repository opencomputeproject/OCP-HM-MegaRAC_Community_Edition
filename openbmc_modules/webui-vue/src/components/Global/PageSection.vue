<template>
  <div class="page-section">
    <h2 v-if="sectionTitle">{{ sectionTitle }}</h2>
    <slot />
  </div>
</template>

<script>
export default {
  name: 'PageSection',
  props: {
    sectionTitle: {
      type: String,
      default: ''
    }
  }
};
</script>

<style lang="scss" scoped>
.page-section {
  margin-bottom: $spacer * 2;
}

h2 {
  @include font-size($h3-font-size);
  margin-bottom: $spacer;
  &::after {
    content: '';
    display: block;
    width: 100px;
    border: 1px solid gray('300');
    margin-top: 10px;
  }
}
</style>
