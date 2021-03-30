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
          variable: 'green-100',
          hex: '#ecfdf1',
        },
        {
          variable: 'green-500',
          hex: '#0a7d06'
        }
      ]
    };
  }
};
</script>

<style lang="scss">
  @import "./colors.scss";
</style>
