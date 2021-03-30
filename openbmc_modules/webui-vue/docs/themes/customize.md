# How to customize
Customization of the application requires knowledge of Sass and CSS. It also will require becoming familiar with the Bootstrap and Bootstrap-Vue component libraries. This section outlines the global options and variables that can be removed or updated to meet organizational brand guidelines.

## Bootstrap Sass Options
The Bootstrap Sass options are global styling toggles. The naming convention for these built-in variables is `enabled-*`.

### $enable-rounded
This option enables or disables the border-radius styles on various components.

- Set to `false` to remove rounded corners on all container elements and buttons

### $enable-validation-icons
This option enables or disables background-image icons within textual inputs and some custom forms for validation states.

- Set to `false` due to inability to style icons using CSS

### More information
- [View all the Bootstrap Sass Options](https://getbootstrap.com/docs/4.2/getting-started/theming/#sass-options)

## Bootstrap Sass Variables
These are global variables that Bootstrap defines in the `/node_modules/bootstrap/scss/variables.scss` helper file. Adding a variable listed in this file to the `/src/assets/styles/bmc/helpers/_variables.scss` file will override the Bootstrap defined value.

### $transition-base
This variable sets the base CSS transition style throughout the application.
- Set to `all $duration--moderate-02 $standard-easing--productive`

### $transition-fade
This variable sets the transition when showing and hiding elements.

- Set to `opacity $duration--moderate-01 $standard-easing--productive`

### $transition-collapse
This variable sets the CSS transitions throughout the application when expanding and collapsing elements.

- Set to `height $duration--slow-01 $standard-easing--expressive`

### More Information
- [Carbon Design System Motion Guidelines](https://www.carbondesignsystem.com/guidelines/motion/basics/)
- [Including Animation In Your Design System](https://www.smashingmagazine.com/2019/02/animation-design-system/)

## OpenBMC Custom Sass Options

### $responsive-layout-bp
This variable determines when the primary navigation is hidden and when the hamburger menu is displayed. The breakpoint is defined using a Bootstrap function that only accepts a key from the Bootstrap `$grid-breakpoints` map.

- xs - Navigation is always displayed
- sm - Navigation displayed when the viewport is greater than 576px
- md - Navigation displayed when the viewport is greater than 768px
- lg - Navigation displayed when the viewport is greater than 992px
- xl - Navigation displayed when the viewport is less than 1200px

#### Responsive Resources
- [Bootstrap responsive breakpoints](https://getbootstrap.com/docs/4.0/layout/overview/#responsive-breakpoints)
- [Bootstrap Sass Mixins](https://getbootstrap.com/docs/4.0/layout/overview/#responsive-breakpoints)
- [Customizing the Bootstrap Grid](https://getbootstrap.com/docs/4.0/layout/overview/#responsive-breakpoints)

### $header-height
This variable determines the height of the OpenBMC Web UI header element.

- Default value: 56px

### $navigation-width
This variable determines the width of the primary navigation menu when expanded.

- Default value: 300px

### $container-bgd
This option sets the background of page containers. Changing the value of this variable will change the background color for the following elements:
-  Login page
- Primary navigation section
- Quick links section on the overview page

#### Value
- Default value: $gray-200

### $primary-nav-hover
The semantic naming of this variable identifies its purpose. This color should always be slightly darker than the `$container-bgd` value.

- Default value: $gray-300

## Updating Colors
Supporting a different color palette is a simple process. The default color palette is supported using the Sass variables outlined in the color guidelines and color maps outlined in the theme's overview.  The following sections provide directions to update the settings to meet your organization's needs.

### Color
The OpenBMC Web UI uses Sass variables and maps to build its layout and components. Bootstrap variables and maps use the `!default` flag to allow for overrides. There are three Sass maps created to establish the color palette. These include the `color` map, `theme-color` map, and `gray` map. These maps are used by Bootstrap to build the application's CSS stylesheets.

#### All Colors
The OpenBMC Web UI custom colors are available as Sass variables and a Sass map in `/src/assets/styles/bmc/helpers/_variables.scss`. The OpenBMC theme only requires a subset of the colors to create the look and feel of the application. Adding, removing, or updating the color variables and map is how the application color palette can be customized. Using these variables outside of the helper files is discouraged to avoid tightly coupling the OpenBMC Web UI theme to specific colors.

The `color` map is not as important as the `theme-color` map. A tight-coupling of the Sass variable name to the color name makes it hard to use the `color` map keys for customization. Using these keys in Sass stylesheets or single-file components is discouraged.

#### Theme Colors
The theme color variables and the `theme-color` map consist of a subset of the color variables. This smaller color palette creates a scheme that is not dependent on specific colors like blue or green. Several of the Bootstrap `theme-color` map keys are required to generate the CSS styles. The OpenBMC Web UI `theme-color` map has the same keys as the Bootstrap `theme-color` map with custom values.

The `theme-color` map is used heavily throughout the application. The Bootstrap-Vue components `variant` prop also utilizes the `theme-color` map. This map is the key to customizing the application's color palette. Take a look at the [color guidelines](/guide/guidelines/colors) to better understand default theme-colors map.

#### Gray Colors
The gray color palette contains nine shades of gray that range from light to dark. Bootstrap sets a default gray color value for each color variable from 100-900 in increments of one hundred, for example, `$gray-100`, `$gray-200`, `$gray-300` through `gray-900`. Bootstrap does not create a color map for any of the colors except gray. The Bootstrap documentation indicates that adding color maps for all the default colors is scheduled to be delivered in a future patch. The OpenBMC Web UI color theme overrides all shades of the Bootstrap default gray variable values.

[Learn more about Bootstrap colors](https://getbootstrap.com/docs/4.0/getting-started/theming/#color)

### Bootstrap Color Functions
- `color('<color map key>)`
- `theme-color('<theme color map key>)`
- `gray('<gray color palette key'>)`


#### Example
```SCSS
.some-selector {
  color: color("blue");
  background: theme-color("light");
  border: 1px solid gray("900")
}
```

[Learn more about using Bootstrap functions](https://getbootstrap.com/docs/4.0/getting-started/theming/#functions)
## Adding a logo
The updated page header can include a small logo. The guidelines for adding a logo and the suggested logo dimensions are currently in progress. It may be challenging to meet all organization brand guidelines due to the minimal height of the page header. The company logo might be able to be set in the primary navigation, but a design supporting that option will be the focus of future design work.
