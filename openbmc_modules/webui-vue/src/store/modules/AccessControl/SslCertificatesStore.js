import api from '../../api';
import i18n from '../../../i18n';

export const CERTIFICATE_TYPES = [
  {
    type: 'HTTPS Certificate',
    location: '/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/',
    label: i18n.t('pageSslCertificates.httpsCertificate')
  },
  {
    type: 'LDAP Certificate',
    location: '/redfish/v1/AccountService/LDAP/Certificates/',
    label: i18n.t('pageSslCertificates.ldapCertificate')
  },
  {
    type: 'TrustStore Certificate',
    location: '/redfish/v1/Managers/bmc/Truststore/Certificates/',
    // Web UI will show 'CA Certificate' instead of
    // 'TrustStore Certificate' after user testing revealed
    // the term 'TrustStore Certificate' wasn't recognized/was unfamilar
    label: i18n.t('pageSslCertificates.caCertificate')
  }
];

const getCertificateProp = (type, prop) => {
  const certificate = CERTIFICATE_TYPES.find(
    certificate => certificate.type === type
  );
  return certificate ? certificate[prop] : null;
};

const SslCertificatesStore = {
  namespaced: true,
  state: {
    allCertificates: [],
    availableUploadTypes: []
  },
  getters: {
    allCertificates: state => state.allCertificates,
    availableUploadTypes: state => state.availableUploadTypes
  },
  mutations: {
    setCertificates(state, certificates) {
      state.allCertificates = certificates;
    },
    setAvailableUploadTypes(state, availableUploadTypes) {
      state.availableUploadTypes = availableUploadTypes;
    }
  },
  actions: {
    async getCertificates({ commit }) {
      return await api
        .get('/redfish/v1/CertificateService/CertificateLocations')
        .then(({ data: { Links: { Certificates } } }) =>
          Certificates.map(certificate => certificate['@odata.id'])
        )
        .then(certificateLocations => {
          const promises = certificateLocations.map(location =>
            api.get(location)
          );
          api.all(promises).then(
            api.spread((...responses) => {
              const certificates = responses.map(({ data }) => {
                const {
                  Name,
                  ValidNotAfter,
                  ValidNotBefore,
                  Issuer = {},
                  Subject = {}
                } = data;
                return {
                  type: Name,
                  location: data['@odata.id'],
                  certificate: getCertificateProp(Name, 'label'),
                  issuedBy: Issuer.CommonName,
                  issuedTo: Subject.CommonName,
                  validFrom: new Date(ValidNotBefore),
                  validUntil: new Date(ValidNotAfter)
                };
              });
              const availableUploadTypes = CERTIFICATE_TYPES.filter(
                ({ type }) =>
                  !certificates
                    .map(certificate => certificate.type)
                    .includes(type)
              );

              commit('setCertificates', certificates);
              commit('setAvailableUploadTypes', availableUploadTypes);
            })
          );
        });
    },
    async addNewCertificate({ dispatch }, { file, type }) {
      return await api
        .post(getCertificateProp(type, 'location'), file, {
          headers: { 'Content-Type': 'application/x-pem-file' }
        })
        .then(() => dispatch('getCertificates'))
        .then(() =>
          i18n.t('pageSslCertificates.toast.successAddCertificate', {
            certificate: getCertificateProp(type, 'label')
          })
        )
        .catch(error => {
          console.log(error);
          throw new Error(
            i18n.t('pageSslCertificates.toast.errorAddCertificate')
          );
        });
    },
    async replaceCertificate(
      { dispatch },
      { certificateString, location, type }
    ) {
      const data = {};
      data.CertificateString = certificateString;
      data.CertificateType = 'PEM';
      data.CertificateUri = { '@odata.id': location };

      return await api
        .post(
          '/redfish/v1/CertificateService/Actions/CertificateService.ReplaceCertificate',
          data
        )
        .then(() => dispatch('getCertificates'))
        .then(() =>
          i18n.t('pageSslCertificates.toast.successReplaceCertificate', {
            certificate: getCertificateProp(type, 'label')
          })
        )
        .catch(error => {
          console.log(error);
          throw new Error(
            i18n.t('pageSslCertificates.toast.errorReplaceCertificate')
          );
        });
    },
    async deleteCertificate({ dispatch }, { type, location }) {
      return await api
        .delete(location)
        .then(() => dispatch('getCertificates'))
        .then(() =>
          i18n.t('pageSslCertificates.toast.successDeleteCertificate', {
            certificate: getCertificateProp(type, 'label')
          })
        )
        .catch(error => {
          console.log(error);
          throw new Error(
            i18n.t('pageSslCertificates.toast.errorDeleteCertificate')
          );
        });
    },
    async generateCsr(_, userData) {
      const {
        certificateType,
        country,
        state,
        city,
        companyName,
        companyUnit,
        commonName,
        keyPairAlgorithm,
        keyBitLength,
        keyCurveId,
        challengePassword,
        contactPerson,
        emailAddress,
        alternateName
      } = userData;
      const data = {};

      data.CertificateCollection = {
        '@odata.id': getCertificateProp(certificateType, 'location')
      };
      data.Country = country;
      data.State = state;
      data.City = city;
      data.Organization = companyName;
      data.OrganizationalUnit = companyUnit;
      data.CommonName = commonName;
      data.KeyPairAlgorithm = keyPairAlgorithm;
      data.AlternativeNames = alternateName;

      if (keyCurveId) data.KeyCurveId = keyCurveId;
      if (keyBitLength) data.KeyBitLength = keyBitLength;
      if (challengePassword) data.ChallengePassword = challengePassword;
      if (contactPerson) data.ContactPerson = contactPerson;
      if (emailAddress) data.Email = emailAddress;

      return await api
        .post(
          '/redfish/v1/CertificateService/Actions/CertificateService.GenerateCSR',
          data
        )
        //TODO: Success response also throws error so
        // can't accurately show legitimate error in UI
        .catch(error => console.log(error));
    }
  }
};

export default SslCertificatesStore;
