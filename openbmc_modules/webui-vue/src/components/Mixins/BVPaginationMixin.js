const BVPaginationMixin = {
  data() {
    return {
      currentPage: 1,
      perPage: 20,
      itemsPerPageOptions: [
        {
          value: 10,
          text: '10'
        },
        {
          value: 20,
          text: '20'
        },
        {
          value: 30,
          text: '30'
        },
        {
          value: 40,
          text: '40'
        },
        {
          value: 0,
          text: this.$t('global.table.viewAll')
        }
      ]
    };
  },
  methods: {
    getTotalRowCount(count) {
      return this.perPage === 0 ? 0 : count;
    }
  }
};

export default BVPaginationMixin;
