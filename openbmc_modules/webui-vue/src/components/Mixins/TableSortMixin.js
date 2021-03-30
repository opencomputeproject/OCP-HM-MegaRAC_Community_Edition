const STATUS = ['OK', 'Warning', 'Critical'];

const TableSortMixin = {
  methods: {
    sortStatus(a, b, key) {
      return STATUS.indexOf(a[key]) - STATUS.indexOf(b[key]);
    }
  }
};

export default TableSortMixin;
