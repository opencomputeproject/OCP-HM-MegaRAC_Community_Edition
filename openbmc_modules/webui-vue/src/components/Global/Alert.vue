<template>
  <b-alert :show="show" :variant="variant" :class="{ small }">
    <div v-if="variant == 'warning' || variant == 'danger'" class="alert-icon">
      <status-icon :status="variant" />
    </div>
    <div class="alert-content d-inline-flex align-items-center">
      <div class="alert-msg"><slot /></div>
    </div>
    <div class="alert-action d-inline-flex align-items-center">
      <slot name="action"></slot>
    </div>
  </b-alert>
</template>

<script>
import StatusIcon from '../Global/StatusIcon';
import { BAlert } from 'bootstrap-vue';

export default {
  name: 'Alert',
  components: {
    BAlert: BAlert,
    StatusIcon: StatusIcon
  },
  props: {
    show: {
      type: Boolean,
      default: true
    },
    variant: {
      type: String,
      default: ''
    },
    small: Boolean
  }
};
</script>
