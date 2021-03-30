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
          variable: 'gray-100',
          hex: '#fafafa',
          border: true
        },
        {
          variable: 'gray-200',
          hex: '#f4f4f4'
        },
        {
          variable: 'gray-300',
          hex: '#dcdee0'
        },
        {
          variable: 'gray-400',
          hex: '#cccccc'
        },
        {
          variable: 'gray-500',
          hex: '#b3b3b3'
        },
        {
          variable: 'gray-600',
          hex: '#999999'
        },
        {
          variable: 'gray-700',
          hex: '#666666'
        },
        {
          variable: 'gray-800',
          hex: '#333333'
        },
        {
          variable: 'gray-900',
          hex: '#161616'
        }
      ]
    };
  }
};
</script>

<style lang="scss">
  @import "./colors.scss";
</style>
