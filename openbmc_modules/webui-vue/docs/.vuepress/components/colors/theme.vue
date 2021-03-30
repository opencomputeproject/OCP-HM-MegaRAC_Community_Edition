<template>
  <div>
    <div class="color-tile-container">
      <div v-for="item in themeColors">
        <div
          :style="{ backgroundColor: item.hex }"
          :class="{ 'color-tile--border': item.border }"
          class="color-tile"
        ></div>
        <dl class="color-tile-desc">
          <dt>Color variable:</dt>
          <dd>${{ item.name }}</dd>
        </dl>
        <dl class="color-tile-desc">
          <dt>Color variable:</dt>
          <dd>${{ item.variable }}</dd>
        </dl>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  data() {
    return {
      themeColors: [
        {
          name: "primary",
          variable: "blue",
          hex: "#2d60e5"
        },
        {
          name: "secondary",
          variable: "gray-800",
          hex: "#333333"
        },
        {
          name: 'light',
          variable: 'gray-100',
          hex: '#fafafa',
          border: true
        },
        {
          name: 'dark',
          variable: 'gray-900',
          hex: '#161616'
        },
        {
          name: 'info',
          variable: 'blue',
          hex: '#2d60e5'
        },
        {
          name: 'success',
          variable: 'green',
          hex: '#0a7d06'
        },
        {
          name: 'warning',
          variable: 'yellow',
          hex: '#efc100'
        },
        {
          name: 'danger',
          variable: 'red',
          hex: '#da1416'
        }
      ]
    };
  }
};
</script>

<style lang="scss">
  @import "./colors.scss";
</style>
