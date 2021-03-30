<template>
  <b-link
    class="btn btn-primary d-block align-self-center"
    :download="download"
    :href="href"
  >
    {{ $t('global.action.export') }}
  </b-link>
</template>

<script>
export default {
  props: {
    data: {
      type: Array,
      default: () => []
    },
    fileName: {
      type: String,
      default: 'data'
    }
  },
  computed: {
    dataForExport() {
      return JSON.stringify(this.data);
    },
    download() {
      return `${this.fileName}.json`;
    },
    href() {
      return `data:text/json;charset=utf-8,${this.dataForExport}`;
    }
  }
};
</script>
