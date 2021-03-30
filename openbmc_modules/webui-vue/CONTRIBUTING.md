Contributing to WebUI-Vue

:+1::tada: First off, thanks for taking the time to contribute! :+1::tada:

The following is a set of guidelines for contributing to the OpenBMC Web UI.

## Table of contents
- [Project Setup](#project-setup)
- [Code of Conduct](#code-of-conduct)
- [Asking Questions](#asking-questions)
- [Submitting Bugs](#submitting-bugs)
- [Request a New Feature](#request-a-new-feature)
- [User Research](#user-research)
- [Design Reviews](#design-reviews)
- [Help Wanted](#help-wanted)
- [Code Reviews](#code-reviews)

## Project Setup

### Set up Gerrit account
The OpenBMC projects use Gerrit for code review. Use the [OpenBMC Gerrit Setup/Integration guidelines](https://github.com/openbmc/docs/blob/master/development/gerrit-setup.md) to set up your Gerrit account.

### Clone repo from Gerrit
Since we use Gerrit to manage all the code contributions, it is not necessary to create a fork from GitHub. Read more about submitting a code review in the [Code Reviews section](#code-reviews) below.

1. Visit https://gerrit.openbmc-project.xyz/admin/repos/openbmc/webui-vue
2. Clone the repo using the `Clone with commit-msg hook` link
    - Windows users may need to separate the two commands depending on if they have installed a Bash emulator like `Git for Windows` or `CMDER` or installed the `Windows Subsystem for Linux`.

### Install Dependencies
```
npm install
```

### Create a .env file
1. Create the following file in the root directory
     - .env.development.local
1. Add the following environment variable
    - BASE_URL="https://<BMC IP address or FQDN>"`


### Compile and hot-reload for development
```
npm run serve
```

### Run Documentation Locally (Vuepress static site generation)
```
Run npm docs:serve
```

### Run unit tests
```
npm run test:unit
```

### Lint and fix files
```
npm run lint
```

This project and everyone participating are required to follow the [OpenBMC Code of Conduct](https://github.com/openbmc/docs/blob/master/code-of-conduct.md).

## Asking Questions
The OpenBMC communicates using a mail list and an IRC channel. Although you could create a GitHub issue to ask a question, a better option would be to ask your question using the email list or IRC channel.

- Mail: openbmc@lists.ozlabs.org https://lists.ozlabs.org/listinfo/openbmc
- IRC: #openbmc on freenode.net

### Submitting Bugs
Before submitting an issue, please search through the [existing GitHub issues](https://github.com/openbmc/webui-vue/issues) to see if someone already reported the problem. If the issue does not exist, use the [Bug report template](https://github.com/openbmc/webui-vue/issues/new?assignees=&labels=&template=bug_report.md&title=) to report the problem.

Before submitting a request for a new feature, please search through the [existing GitHub issues](https://github.com/openbmc/webui-vue/issues). If no one has suggested the idea previously, use the [Feature request template](https://github.com/openbmc/webui-vue/issues/new?assignees=&labels=&template=feature_request.md&title=) to submit a new feature request.

## User Research
To improve the OpenBMC Web UI, we are actively seeking end-user feedback through the IBM Design Partner program.

### What is an OpenBMC Design Partner?
Contribute your feedback and bring your domain expertise to help us improve the OpenBMC GUI. The time commitment is minimal: 1-hour feedback sessions conducted remotely via Webex, 1 to 2 times per month. Activities may include exploratory interviews, participatory design, periodic surveys, or usability testing sessions.

Note: IBM requires all user research participants to sign a Feedback Program Agreement (FPA), describing how IBM complies with the latest GDPR guidelines. The FPA only needs to be signed once to cover your company or entity's participation.


### How do I become a Design Partner?
If you are using the OpenBMC GUI and are interested in influencing its look and feel, email nicoleconser@ibm.com with the subject line `OpenBMC GUI Design Partner Referral`.

## Design Reviews
When making changes to an existing design, we create a design review issue in GitHub and add it to the Design Reviews Project](https://github.com/openbmc/webui-vue/projects/1). We then send an email to the community and review the changes in the [OpenBMC GUI Design Workgroup](https://github.com/openbmc/openbmc/wiki/GUI-Design-work-group.

## Help Wanted
New features and bugs ready for development will be tagged with the `Help Wanted` tag and added to the [Help Wanted Project](https://github.com/openbmc/webui-vue/projects/2). If you want to pick up a story, add a comment stating your interest, and a maintainer will assign the story and move it to the `In Progress` column in the Project Kanban board.

## Code Reviews
The OpenBMC project uses Gerrit for all code reviews. Please do not create a GitHub pull request.

1. Commit your changes, adding a Signed-off-by line to it:  `git commit --signoff`
1. Write a [good commit message](https://chris.beams.io/posts/git-commit/)
    - Separate subject from the body with a blank line
    - Limit the subject line to 50 characters
    - Capitalize the subject line
    - Do not end the subject line with a period
    - Use the imperative mood in the subject line
    - Wrap the body at 72 characters
    - Use the body to explain what and why vs. how

1. Push your changes to Gerrit for code review: `git push origin HEAD:refs/for/master`
1. Assign reviewers



