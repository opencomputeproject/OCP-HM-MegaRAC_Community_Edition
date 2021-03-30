# Configuring environment specific builds

This document provides instructions for how to add environment specific modifications to the Web UI.

- [Setup](#setup)
- [Store](#store)
- [Router](#router)
- [Theming](#theming)
- [Conditional template rendering](#conditional-template-rendering)
- [Local development](#local-development)
- [Production build](#production-build)

## Setup

1. Create a `.env.<ENV_NAME>` file in the project root
2. Add `NODE_ENV=production` key value pair in the file
3. Add `VUE_APP_ENV_NAME` key with the value set to the new environment name

Example `.env.openpower`:

```
NODE_ENV=production
VUE_APP_ENV_NAME=openpower
```

## Store

>[Vuex store modules](https://vuex.vuejs.org/guide/modules.html) contain the application's API calls.

1. Create a `<ENV_NAME>.js` file in `src/env/store`
    >The filename needs to match the `VUE_APP_ENV_NAME` value defined in the .env file. The store import in `src/main.js` will resolve to this new file.
2. Import the base store
3. Import environment specific store modules
4. Use the [Vuex](https://vuex.vuejs.org/api/#registermodule) `registerModule` and `unregisterModule` instance methods to add/remove store modules
5. Add default export

Example `src/env/store/openpower.js`:

```
import store from '@/store; //@ aliases to src directory
import HmcStore from './Hmc/HmcStore';

store.registerModule('hmc', HmcStore);

export default store;
```

## Router

>[Vue Router](https://router.vuejs.org/guide/) determines which pages are accessible in the UI.

1. Create a `<ENV_NAME>.js` file in `src/env/router`
    >The filename needs to match the `VUE_APP_ENV_NAME` value defined in the .env file. The router import in `src/main.js` will resolve to this new file.
2. Import the base router
4. Use the [Vue Router](https://router.vuejs.org/api/#router-addroutes) `addRoutes` instance method to define new routes
5. Add default export

Example `src/env/router/openpower.js`:

```
import router from '@/router'; //@ aliases to src directory
import AppLayout from '@/layouts/AppLayout';

router.addRoutes([
  {
    path: '/',
    component: AppLayout,
    children: [
      {
        path: '/access-control/hmc',
        component: () => import('../views/Hmc'),
        meta: {
          title: 'appPageTitle.hmc'
        }
      }
    ]
  }
]);

export default router;
```

## Theming

>[Bootstrap theming](https://getbootstrap.com/docs/4.5/getting-started/theming/) allows for easy visual customizations.

1. Create a `_<ENV_NAME>.scss` partial in `src/env/assets/styles`
    >The filename needs to match the `VUE_APP_ENV_NAME` value defined in the .env file. The webpack sass loader will attempt to import a file with this name.
2. Add style customizations. Refer to [bootstrap documentation](https://getbootstrap.com/docs/4.5/getting-started/theming/) for details about [variable override options](https://getbootstrap.com/docs/4.5/getting-started/theming/#sass-options) and [theme color maps](https://getbootstrap.com/docs/4.5/getting-started/theming/#maps-and-loops).

Example for adding custom colors

`src/env/assets/styles/_openpower.scss`

```
// Custom theme colors
$theme-colors: (
  "primary": rebeccapurple,
  "success": lime
);
```

## Conditional template rendering

For features that show or hide chunks of code in the template/markup, use the src/`envConstants.js` file, to determine which features are enabled/disabled depending on the `VUE_APP_ENV_NAME` value.

>Avoid complex v-if/v-else logic in the templates. If a template is being **heavily** modified, consider creating a separate View and [updating the router definition](#router).

1. Add the environment specific feature name and value in the `envConstants.js` file
2. Import the ENV_CONSTANTS object in the component needing conditional rendering
3. Use v-if/else as needed in the component template

Example for adding conditional navigation item to AppNavigation.vue component:

`src/envConstants.js`

```
const envName = process.env.VUE_APP_ENV_NAME;

export const ENV_CONSTANTS = {
  name: envName || 'openbmc',
  hmcEnabled: envName === 'openpower' ? true : false
};

```

`src/components/AppNavigation/AppNavigation.vue`


```
<template>

...

  <b-nav-item
    to="/access-control/hmc"
    v-if="hmcEnabled">
    HMC
  </b-nav-item>

...

</template>

<script>
import { ENV_CONSTANTS } from '@/envConstants.js';

export default {
  data() {
    return {
      hmcEnabled: ENV_CONSTANTS.hmcEnabled
    }
  }
}

</script>

```

## Local development

1. Add the same `VUE_APP_ENV_NAME` key value pair to your `env.development.local` file.
2. Use serve script
    ```
    npm run serve
    ```

## Production build

Run npm build script with vue-cli `--mode` [option flag](https://cli.vuejs.org/guide/mode-and-env.html#modes). This requires [corresponding .env file to exist](#setup).


```
npm run build -- --mode openpower
```


**OR**

pass env variable directly to script

```
VUE_APP_ENV_NAME=openpower npm run build
```