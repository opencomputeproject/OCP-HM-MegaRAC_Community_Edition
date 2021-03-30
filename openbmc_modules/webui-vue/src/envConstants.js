const envName = process.env.VUE_APP_ENV_NAME;

export const ENV_CONSTANTS = {
  name: envName || 'openbmc'
};
