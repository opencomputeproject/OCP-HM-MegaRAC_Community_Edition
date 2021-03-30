module.exports = {
    base: "/webui-vue/",
    title: "OpenBMC Web UI Style Guide",
    description:
      "Guidance on code style and development for the OpenBMC browser-based UI",
    smoothScroll: true,
    themeConfig: {
      nav: [
        {
          text: "Guide",
          link: "/guide/"
        },
        {
          text: "Themes",
          link: "/themes/"
        },
        {
          text: "Github",
          link: "https://github.com/openbmc/webui-vue"
        }
      ],
      sidebarDepth: 1,
      sidebar: {
        "/guide/": [
          "",
          {
            title: "Coding Standards",
            children: [
              "/guide/coding-standards/",
              ["/guide/coding-standards/accessibility", "Accessibility"],
              ["/guide/coding-standards/sass", "SASS"],
              ["/guide/coding-standards/javascript", "JavaScript"]
            ]
          },
          {
            title: "Guidelines",
            children: [
              "/guide/guidelines/",
              "/guide/guidelines/colors",
              "/guide/guidelines/motion",
              "/guide/guidelines/typography"
            ]
          },
          {
            title: "Components",
            children: [
            "/guide/components/",
            "/guide/components/alert",
            "/guide/components/button",
            "/guide/components/toast",
          ]
          }
        ],
        "/themes/": ["", "customize"]
      },
    }
  };