<template>
  <div class="search-global">
    <b-form-group
      :label="$t('global.form.search')"
      :label-for="`searchInput-${_uid}`"
      label-sr-only
      class="mb-2"
    >
      <b-input-group size="md" class="align-items-center">
        <b-input-group-prepend>
          <icon-search class="search-icon" />
        </b-input-group-prepend>
        <b-form-input
          :id="`searchInput-${_uid}`"
          v-model="filter"
          class="search-input"
          type="text"
          :placeholder="placeholder"
          @input="onChangeInput"
        ></b-form-input>
      </b-input-group>
    </b-form-group>
  </div>
</template>

<script>
import IconSearch from '@carbon/icons-vue/es/search/16';
export default {
  name: 'Search',
  components: { IconSearch },
  props: {
    placeholder: {
      type: String,
      default: function() {
        return this.$t('global.form.search');
      }
    }
  },
  data() {
    return {
      filter: null
    };
  },
  methods: {
    onChangeInput() {
      this.$emit('changeSearch', this.filter);
    }
  }
};
</script>

<style lang="scss" scoped>
.search-input {
  padding-left: ($spacer * 2);
}
.search-icon {
  position: absolute;
  left: 10px;
  top: 12px;
  z-index: 4;
  stroke: gray('400');
}
</style>
