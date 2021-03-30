<template>
    <div>
        <h3 class="h4">Enabled</h3>
        <b-button variant="primary">Primary</b-button>
        <b-button variant="primary">
            <icon-add />
            <span>Primary with icon</span>
        </b-button>
        <b-button variant="secondary">Secondary</b-button>
        <b-button variant="danger">Danger</b-button>
        <b-button variant="link">Link Button</b-button>
        <b-button variant="link">
            <icon-add />
            <span>Link Button</span>
        </b-button>

        <h3 class="h4">Disabled</h3>
        <b-button disabled variant="primary">Primary</b-button>
        <b-button disabled variant="primary">
            <icon-add />
            <span>Primary with icon</span>
        </b-button>
        <b-button disabled variant="secondary">Secondary</b-button>
        <b-button disabled variant="danger">Danger</b-button>
        <b-button disabled variant="link">Link Button</b-button>
        <b-button disabled variant="link">
            <icon-add />
            <span>Link Button</span>
        </b-button>
    </div>
</template>

<script>
import IconAdd from '@carbon/icons-vue/es/add--alt/20';
import IconArrowRight from '@carbon/icons-vue/es/arrow--right/16';
export default {
    name: 'BmcButtons',
    components: { IconAdd, IconArrowRight }
}
</script>
<style scoped>
    button {
        margin-bottom: 1rem;
        margin-right: .5rem;
    }

    h3 {
        margin: .5rem 0 1rem;
    }
</style>