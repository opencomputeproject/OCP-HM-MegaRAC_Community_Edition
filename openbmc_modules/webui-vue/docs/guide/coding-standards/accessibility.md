# Accessibility Conventions and Standards

It is important that the OpenBMC Web UI meet accessibility guidelines established by the [World Wide Web Consortium (W3C)](https://www.w3.org/). These guidelines are known as the [Web Content Accessibility Guidelines (WCAG)](https://www.w3.org/WAI/standards-guidelines/wcag/). Making the Web UI accessible to as many users as possible is the right thing to do. In many countries, it is also legally required. Organizations providing interfaces that users with permanent or temporary disabilities can not use, may lose sales or be susceptible to discriminatory lawsuits.

## Accessibility Principles
These [principles](https://www.w3.org/WAI/fundamentals/accessibility-principles/) reference a set of international standards from the [W3C Web Accessibility Intitiative (WAI)](https://www.w3.org/WAI/).

- [Perceiveable](https://www.w3.org/WAI/fundamentals/accessibility-principles/#perceivable)
- [Operable](https://www.w3.org/WAI/fundamentals/accessibility-principles/#operable)
- [Understandable](https://www.w3.org/WAI/fundamentals/accessibility-principles/#understandable)
- [Robust](https://www.w3.org/WAI/fundamentals/accessibility-principles/#robust)

## Semantic HTML
Coding the UI using semantic markup is the most important step in creating an inclusive interface. The use of [WAI-ARIA](https://www.w3.org/WAI/standards-guidelines/aria/) can help make an interface accessible to assistive technologies. However, there are two critical rules to follow:

1. Always favor semantic markup over ARIA
2. No ARIA is better than Bad ARIA

## Testing
Assuring the Web UI meets accessibility guidelines requires a combination of automated and manual testing. Automated tests will test the application against common problems such as color contrast and ARIA use. Automated testing can be built into the CI process, integrated with code editors, and run using browser extensions.

The OpenBMC Web UI developers should test their development pages using one of the tools listed in the tools section below. If using Chrome, the Lighthouse application comes bundled with the browser and also includes testing for performance and best practices. If there is an issue that is created when using a Bootstrap-Vue component, we can [create an issue in the Bootstrap-vue repo](https://github.com/bootstrap-vue/bootstrap-vue/issues/new/choose). Contributing to the Bootstrap-Vue open-source library, when possible, is strongly encouraged.

## Tools
- [Deque Axe](https://www.deque.com/axe/)
- [Firefox Accessibility Inspector](https://developer.mozilla.org/en-US/docs/Tools/Accessibility_inspector)
- [IBM Accessibility Tools](https://www.ibm.com/able/toolkit/tools)
- [Lighthouse](https://developers.google.com/web/tools/lighthouse)

## Screen Readers
- [Voiceover (Mac only)](https://webaim.org/articles/voiceover/)
- [NVDA (Windows only)](https://webaim.org/articles/nvda/)
- [Jaws - (Windows only)](https://webaim.org/articles/jaws/)

## Resources
- [Mozilla Developer Network - Accessibility](https://developer.mozilla.org/en-US/docs/Web/Accessibility)
- [Web Content Accessibility Guidelines (WCAG)](https://www.w3.org/WAI/standards-guidelines/wcag/)
- [WAI-ARIA Authoring Practices](https://www.w3.org/TR/wai-aria-practices/)
- [WAI-ARIA Basics](https://developer.mozilla.org/en-US/docs/Learn/Accessibility/WAI-ARIA_basics)
- [WebAIM Articles](https://webaim.org/articles/)
- [A11Y Project](https://a11yproject.com/)
- [IBM Accessibility](https://www.ibm.com/able/)
- [Inclusive Components](https://inclusive-components.design/)

