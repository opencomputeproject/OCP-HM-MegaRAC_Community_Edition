<template>
  <div class="table-filter d-inline-block">
    <p class="d-inline-block mb-0">
      <b-badge v-for="(tag, index) in tags" :key="index" pill>
        {{ tag }}
        <b-button-close
          :disabled="dropdownVisible"
          :aria-hidden="true"
          @click="removeTag(tag)"
        />
      </b-badge>
    </p>
    <b-dropdown
      variant="link"
      no-caret
      right
      data-test-id="tableFilter-dropdown-options"
      @hide="dropdownVisible = false"
      @show="dropdownVisible = true"
    >
      <template v-slot:button-content>
        <icon-filter />
        {{ $t('global.action.filter') }}
      </template>
      <b-dropdown-form>
        <b-form-group
          v-for="(filter, index) of filters"
          :key="index"
          :label="filter.label"
        >
          <b-form-checkbox-group v-model="tags">
            <b-form-checkbox
              v-for="value in filter.values"
              :key="value"
              :value="value"
              :data-test-id="`tableFilter-checkbox-${value}`"
              @change="onChange($event, { filter, value })"
            >
              {{ value }}
            </b-form-checkbox>
          </b-form-checkbox-group>
        </b-form-group>
      </b-dropdown-form>
      <b-dropdown-item-button
        variant="primary"
        data-test-id="tableFilter-button-clearAll"
        @click="clearAllTags"
      >
        {{ $t('global.action.clearAll') }}
      </b-dropdown-item-button>
    </b-dropdown>
  </div>
</template>

<script>
import IconFilter from '@carbon/icons-vue/es/settings--adjust/20';

export default {
  name: 'TableFilter',
  components: { IconFilter },
  props: {
    filters: {
      type: Array,
      default: () => [],
      validator: prop => {
        return prop.every(
          filter => 'label' in filter && 'values' in filter && 'key' in filter
        );
      }
    }
  },
  data() {
    return {
      dropdownVisible: false,
      activeFilters: this.filters.map(({ key }) => {
        return {
          key,
          values: []
        };
      })
    };
  },
  computed: {
    tags: {
      get() {
        return this.activeFilters.reduce((arr, filter) => {
          return [...arr, ...filter.values];
        }, []);
      },
      set(value) {
        return value;
      }
    }
  },
  methods: {
    removeTag(tag) {
      this.activeFilters.forEach(filter => {
        filter.values = filter.values.filter(val => val !== tag);
      });
      this.emitChange();
    },
    clearAllTags() {
      this.activeFilters.forEach(filter => {
        filter.values = [];
      });
      this.emitChange();
    },
    emitChange() {
      this.$emit('filterChange', {
        activeFilters: this.activeFilters
      });
    },
    onChange(
      checked,
      {
        filter: { key },
        value
      }
    ) {
      this.activeFilters.forEach(filter => {
        if (filter.key === key) {
          checked
            ? filter.values.push(value)
            : (filter.values = filter.values.filter(val => val !== value));
        }
      });
      this.emitChange();
    }
  }
};
</script>

<style lang="scss" scoped>
.badge {
  margin-right: $spacer / 2;
}
</style>
