# Toasts
Use a toast message to indicate the status of a user action. For example, a user saves a form successfully, a toast message with the `success` variant is displayed.  If the user action was not successful, a toast message with the `danger` variant is displayed.

There are different transitions for the toast messages. The `success` toast message will auto-hide after 10 seconds. The user must manually dismiss the `informational`, `warning`, and `error` toast messages.  The `BVToastMixin` provides a simple API that generates a toast message that meets the transition guidelines.

<BmcToasts />

```js{5}
// Sample method from Reboot BMC page
rebootBmc() {
  this.$store
  .dispatch('controls/rebootBmc')
  .then(message => this.successToast(message))
  .catch(({ message }) => this.errorToast(message));
}

// Methods used in this example
methods: {
  makeSuccessToast() {
    this.successToast('This is a success toast and will be dismissed after 10 seconds.');
  },
  makeErrorToast() {
    this.errorToast('This is an error toast and must be dismissed by the user.');
  },
  makeWarningToast() {
    this.warningToast('This is a warning toast and must be dismissed by the user.');
  },
  makeInfoToast() {
    this.infoToast('This is an info toast and must be dismissed by the user.');
  },
}
```