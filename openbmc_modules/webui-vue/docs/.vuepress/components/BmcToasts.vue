<template>
    <div>
        <b-button variant="success" @click="makeSuccessToast()">Show Success</b-button>
        <b-button variant="danger" @click="makeErrorToast()">Show Error</b-button>
        <b-button variant="warning" @click="makeWarningToast()">Show Warning</b-button>
        <b-button variant="info" @click="makeInfoToast()">Show Info</b-button>
    </div>
</template>

<script>
import BVToastMixin from '../../../src/components/Mixins/BVToastMixin';
export default {
    name: 'BmcToasts',
    mixins: [BVToastMixin],
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
}
</script>
<style scoped>
    button {
        margin-right: .5rem;
        margin-bottom: 1rem;
    }
</style>