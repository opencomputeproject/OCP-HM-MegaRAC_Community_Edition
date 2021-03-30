<template>
  <b-container fluid="xl">
    <page-title :description="$t('pageUnauthorized.description')" />
  </b-container>
</template>
<script>
import PageTitle from '../../components/Global/PageTitle';
export default {
  name: 'Unauthorized',
  components: { PageTitle }
};
</script>
