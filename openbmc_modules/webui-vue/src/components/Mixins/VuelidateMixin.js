const VuelidateMixin = {
  methods: {
    getValidationState(model) {
      const { $dirty, $error } = model;
      return $dirty ? !$error : null;
    }
  }
};

export default VuelidateMixin;
