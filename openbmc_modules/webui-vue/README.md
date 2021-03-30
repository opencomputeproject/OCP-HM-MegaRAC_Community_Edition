# webui-vue

webui-vue is a web-based user interface for the OpenBMC firmware stack built on
[Vue.js](https://vuejs.org/).

## Hold on... What happened to phosphor-webui?

[phosphor-webui](https://github.com/openbmc/phosphor-webui) was built on
AngularJS and [AngularJS goes End of Life](https://www.convective.com/angularjs-end-of-life/)
June 30, 2021, so this repository is hopefully its replacement. At this time,
phosphor-webui still contains more features and you should consider using it.

## When will this new Vue.js application reach feature parity with phosphor-webui?

The current plan is by the end of June 2020, with exception to the following pages:
- Firmware
- SNMP settings
- KVM
- Virtual media

## Why will this application be better?

As mentioned, this application is built using Vue.js, a modern open-source
Model-View-ViewModel JavaScript framework supported by an active community and
strong documentation. It has been architected to allow organizations to easily
update the theme to support their brand. This rewrite takes advantage of
front-end development best practices and does not suffer from some of the
anti-patterns that exist in phosphor-webui today.

## How can I get involved?
- Visit the [CONTRIBUTING.md](CONTRIBUTING.md) for more on how to contribute code
- Review some code in [Gerrit](https://gerrit.openbmc-project.xyz/q/project:openbmc%252Fwebui-vue+status:open)
- Join us in the [GUI design workgroup meeting](https://github.com/openbmc/openbmc/wiki/GUI-Design-work-group).


## Documentation
The documentation for coding standards and components is located in the `docs` directory. It is created using the [VuePress](https://vuepress.vuejs.org/) static site generator. Information about how to write documentation can be found on the [VuePress website](https://vuepress.vuejs.org/).