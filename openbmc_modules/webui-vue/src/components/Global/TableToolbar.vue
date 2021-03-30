<template>
  <transition name="slide">
    <div v-if="isToolbarActive" class="toolbar-container">
      <div class="toolbar-content">
        <p class="toolbar-selected">
          {{ selectedItemsCount }} {{ $t('global.action.selected') }}
        </p>
        <div class="toolbar-actions d-flex">
          <b-button
            v-for="(action, index) in actions"
            :key="index"
            :data-test-id="`table-button-${action.value}Selected`"
            variant="primary"
            class="d-block"
            @click="$emit('batchAction', action.value)"
          >
            {{ action.label }}
          </b-button>
          <slot name="export"></slot>
          <b-button
            variant="primary"
            class="d-block"
            @click="$emit('clearSelected')"
          >
            {{ $t('global.action.cancel') }}
          </b-button>
        </div>
      </div>
    </div>
  </transition>
</template>

<script>
export default {
  name: 'TableToolbar',
  props: {
    selectedItemsCount: {
      type: Number,
      required: true
    },
    actions: {
      type: Array,
      default: () => [],
      validator: prop => {
        return prop.every(action => {
          return (
            action.hasOwnProperty('value') && action.hasOwnProperty('label')
          );
        });
      }
    }
  },
  data() {
    return {
      isToolbarActive: false
    };
  },
  watch: {
    selectedItemsCount: function(selectedItemsCount) {
      if (selectedItemsCount > 0) {
        this.isToolbarActive = true;
      } else {
        this.isToolbarActive = false;
      }
    }
  }
};
</script>

<style lang="scss" scoped>
$toolbar-height: 46px;

.toolbar-container {
  width: 100%;
  position: relative;
  z-index: 5;
}

.toolbar-content {
  height: $toolbar-height;
  z-index: $zindex-dropdown + 1;
  background-color: theme-color('primary');
  color: $white;
  position: absolute;
  left: 0;
  right: 0;
  top: -$toolbar-height;
  display: flex;
  flex-direction: row;
  justify-content: space-between;
}

.toolbar-actions {
  > :last-child {
    position: relative;
    &::before {
      content: '';
      position: absolute;
      height: $toolbar-height / 2;
      border-left: 2px solid $white;
      left: -2px;
      top: $toolbar-height / 4;
    }
  }
}

.toolbar-selected {
  line-height: $toolbar-height;
  margin: 0;
  padding: 0 $spacer;
}

.slide-enter-active {
  transition: transform $duration--moderate-02 $entrance-easing--productive;
}
.slide-leave-active {
  transition: transform $duration--moderate-02 $exit-easing--productive;
}
.slide-enter,
.slide-leave-to {
  transform: translateY($toolbar-height);
}
</style>
