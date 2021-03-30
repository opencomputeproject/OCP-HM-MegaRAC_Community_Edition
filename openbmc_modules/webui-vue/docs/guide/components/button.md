# Buttons

Buttons are used to perform an action. The main buttons in the application are the `primary` and `secondary` buttons. Buttons, like all Boostrap-vue components can be themed by setting the `variant` prop on the component to one of the [theme-color map keys](/guide/guidelines/colors). To create a button that looks like a link, set the variant value to `link`.

[Learn more about Bootstrap-vue buttons](https://bootstrap-vue.js.org/docs/components/button)

<BmcButtons />

```vue
// Enabled Buttons
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
```