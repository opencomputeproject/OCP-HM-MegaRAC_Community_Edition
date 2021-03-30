<template>
  <main>
    <b-container class="login-container" fluid>
      <b-row class="login-row" align-v="center">
        <b-col class="login-branding mt-5 mb-5 text-center" md="6">
          <div class="login-branding__container">
            <img
              class="logo"
              width="200px"
              src="@/assets/images/logo-login.svg"
              :alt="altLogo"
            />
            <h1>OpenBMC</h1>
          </div>
        </b-col>
        <b-col md="6">
          <router-view />
        </b-col>
      </b-row>
    </b-container>
  </main>
</template>

<script>
export default {
  name: 'LoginLayout',
  data() {
    return {
      altLogo: `${process.env.VUE_APP_COMPANY_NAME} logo`
    };
  }
};
</script>

<style lang="scss" scoped>
.login-container {
  @include media-breakpoint-up(md) {
    background: linear-gradient(
      to right,
      theme-color('light') 50%,
      gray('200') 50%
    );
  }
}

.login-row {
  @include media-breakpoint-up(md) {
    min-height: 100vh;
  }
}

.login-branding__container {
  @include media-breakpoint-up(md) {
    float: right;
    margin-right: 4rem;
  }
}
</style>
