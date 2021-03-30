<template>
  <div>
    <div class="color-tile-container">
      <div v-for="color in colors">
        <div
          :style="{ backgroundColor: color.hex }"
          :class="{ 'color-tile--border': color.border }"
          class="color-tile"
        ></div>
        <dl class="color-tile-desc">
          <dt>Color variable:</dt>
          <dd>${{ color.variable }}</dd>
        </dl>
        <dl class="color-tile-desc">
          <dt>Hex:</dt>
          <dd>{{ color.hex }}</dd>
        </dl>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  data() {
    return {
      colors: [
        {
          variable: 'blue-100',
          hex: '#eff2fb',
        },
        {
          variable: 'blue-500',
          hex: '#2d60e5'
        }
      ]
    };
  }
};
</script>

<style lang="scss">
  @import "./colors.scss";
</style>
