<template>
  <span :class="['status-icon', status]">
    <icon-success v-if="status === 'success'" />
    <icon-warning v-else-if="status === 'warning'" />
    <icon-danger v-else-if="status === 'danger'" />
    <icon-secondary v-else />
  </span>
</template>

<script>
import IconCheckmark from '@carbon/icons-vue/es/checkmark--filled/20';
import IconWarning from '@carbon/icons-vue/es/warning--filled/20';
import IconError from '@carbon/icons-vue/es/error--filled/20';
import IconMisuse from '@carbon/icons-vue/es/misuse/20';

export default {
  name: 'StatusIcon',
  components: {
    iconSuccess: IconCheckmark,
    iconDanger: IconMisuse,
    iconSecondary: IconError,
    iconWarning: IconWarning
  },
  props: {
    status: {
      type: String,
      default: ''
    }
  }
};
</script>

<style lang="scss" scoped>
.status-icon {
  vertical-align: text-bottom;
  &.success {
    fill: theme-color('success');
  }
  &.danger {
    fill: theme-color('danger');
  }
  &.secondary {
    fill: gray('600');

    svg {
      transform: rotate(-45deg);
    }
  }
  &.warning {
    fill: theme-color('warning');
  }
}
</style>
