# BMC Certificate management

Certificate management allows to replace the existing certificate and private
key file with another (possibly certification Authority (CA) signed)
certificate and private key file. Certificate management allows the user to
install both the server and client certificates. The REST interface allows to
update the certificate, using an unencrypted certificate and private key file
in .pem format, which includes both private key and signed certificate.

### Signed Certificate upload Design flow(Pre-generated):

- The REST Server copies the certificate and private key file to a temporary
  location.
- REST server should map the URI to the target DBus application (Certs) object.
  The recommendation for the D-Bus application implementing certificate D-Bus
  objects is to use the same path structure as the REST endpoint.
     e.g.:
     - The URI /xyz/openbmc_project/certs/server/https maps to instance
           of the certificate application handling Https server certificate.
     - The URI /xyz/openbmc_project/certs/client/ldap maps to instance
           of the certificate application handling LDAP client certificate.
     - The URI /xyz/openbmc_project/certs/authority/ldap maps to instance
           of the certificate application handling Certificate Autohority
           certificates.
- REST server should call the install method of the certificate application
  instance.
- Certificate manager application also implements d-bus object
  xyz.openbmc_project.Certs.Manager. This includes the collection of
  "certificates specific d-bus objects" installed in the system. This d-bus
  provide option to view the certificate on PEM format and delete the same.
  Refer https://en.wikipedia.org/wiki/Privacy-Enhanced_Mail for details.
- Applications should subscribe the xyz.openbmc_project.Certs.Manager
  to see any new certificate is uploaded or change in the existing
  certificates.
- Certificate manager scope is limited to manage the certificate and impacted
  application is responsible for application specific changes.
- In case of delete action, certificate manager creates a new self signed
  certificate after successful delete (regards only server type certificates)

### REST interface details:

   ```
   url: /xyz/openbmc_project/certs/server/https
   Description: Update https server signed certificate and the private key.
   Method: PUT

   url: /xyz/openbmc_project/certs/server/https/<cert_id>
   Description: Delete https server signed certificate and the private key.
   Method: DELETE

   url: /xyz/openbmc_project/certs/client/ldap
   Description: Update ldap client certificate and the private key.
   Method: PUT

   url: /xyz/openbmc_project/certs/client/ldap/<cert_id>
   Description: Delete ldap client certificate and the private key.
   Method: DELETE

   Return codes

       200  Success
       400  Invalid certificate and private key file.
       405  Method not supported.
       500  Internal server error

   ```

## CSR

### User flow for generating and installing Certificates(CSR Based):
   Certificate Signing Request [CSR](https://en.wikipedia.org/wiki/Certificate_signing_request)
is a message sent from an applicant to a certitificate authority in order to
apply for a digital identity certificate. This section provides the details of
the CSR based certificate user flow.
- The user performs the CSR/create interface
      BMC creates new private key and CSR object which includes CSR information.
- The user performs the CSR/export interface
      Allows the user to export the CSR file which is part of newly created
      CSR object. This can be provided to the CA to create SSL certificate.
- The user perform the certificate upload on appropriate services.
      Example: if trying to replace the HTTPS certificate for a Manager,
      navigate to the Manager’s Certificate object upload interface.
      The Upload method internally  pairs the private key used in the first
      step with the installed certificate.

### Assumptions:
- BMC updates the private key associated to CSR for any new CSR request.
- BMC upload process automatically appends certificate file with system CSR
  private key, for the service which requirs certificate and key.
- CSR based Certificate validation is alway's based on private key in the system.

### CSR Request
- CSR requests initiated through D-Bus are time-consuming and might result
  D-Bus time-out error.
- To overcome the time-out error, parent process is forked and CSR operation
  is performed in the child process so that parent process can return the
  calling thread immediately.
- OpenSSL library is used in generating CSR based on the algorithm type.
- At present supporting generating CSR for only "RSA" algorithm type.
- Parent process registers child process PID and a callback method in the
  sd_event_lopp so that callback method is invoked upon completion
  the CSR request in the child process.
- Callback method invoked creates a CSR object with the status of the CSR
  operation returned from the child process.
- CSR read operation will return the CSR string if status is SUCCESS else throws
  InternalFailure exception to the caller.
- Certificate Manager implements "/xyz/openbmc_project/Certs/CSR/Create"
  interface.
- CSR object created implements "/xyz/openbmc_project/Certs/CSR" interface.
- Caller needs to validate the CSR request parameters.
- Caller need to wait on "InterfacesAdded" signal generated upon creation
  of the CSR object to start reading CSR string.

### Example usage for the GenerateCSR POST request

   ```
   url: /redfish/v1/CertificateService
   Action: #CertificateService.GenerateCSR {
    "City": "HYB",
    "CertificateCollection": "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/",
    "CommonName": "www.company.com",
    "ContactPerson":"myname",
    "AlternativeNames":["mycompany.com","mycompany2.com"],
    "ChallengePassword":"abc123",
    "Email":"xxx@xx.com",
    "GivenName":"localhost",
    "Initials":"G",
    "Country": "IN",
    "KeyCurveId":"0",
    "KeyUsage":["ServerAuthentication","ServerAuthentication"],
    "KeyBitLength": 2048,
    "KeyPairAlgorithm": "RSA",
    "Organization": "ABCD",
    "OrganizationUnit": "XY",
    "State": "TX",
    "SurName": "XX",
    "UnstructuredName": "xxx"
   }
   Description: This is used to perform a certificate signing request.
   Method: POST

  ```

### Additional interfaces:
- CertificateService.ReplaceCertificate
      Allows the user to replace an existing certificate.

### d-bus interfaces:

#### d-bus interface to install certificate and private Key
- Certs application must:
  - validate the certificate and Private key file by checking, if the Private
    key matches the public key in the certificate file.
  - copy the certificate and Public Key file to the service specific path
    based on a configuration file.
  - Reload the listed service(s) for which the certificate is updated.

#### d-bus interface to Delete certificate and Private Key

- certificate manager should provide interface to delete the existing
  certificate.
- Incase of server type certificate deleting a signed certificate will
  create a new self signed certificate and will install the same.

### Boot process
-  certificate management instances should be created based on the system
   configuration.

-  Incase of no Https certificate or invalid Https certificate, certificate
   manager should update the https certificate with self signed certificate.

### Repository:
  phosphor-certificate-manager
### Redfish Certificate Support
#### Certificate Upload
- Certificate Manager implements "xyz.openbmc_project.Certs.Install" interface
  for installing certificates in the system.
- Redfish initiates certificate upload by issuing a POST request on the Redfish
  CertificateCollection with the certificate file. Acceptable body formats are:
  raw pem text or json that is acceptable by action
  [CertificateService.ReplaceCertificate](
  https://www.dmtf.org/sites/default/files/standards/documents/DSP2046_2019.1.pdf)

  For example the HTTPS certificate upload POST request is issued on URI
  "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates"
- Bmcweb receives the POST request and it maps the Redfish URI to the
  corresponding Certificate Manager D-Bus URI.
  e.g: HTTPS certificate collection URI
  /redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates mapped to
  /xyz/openbmc_project/certs/server/https.
- Bmcweb initiates an asynchronous call which invokes the "Install" method of
  the Certificate Manager.
- Certificate Manager "Install" method validates, installs the certificate file
  and creates a Certificate object.
- Certificate Manager initiates Reload of the Bmcweb service to trigger
  configuration reload.
- BMCweb service raises SIGHUP signal as part of Reload.
- Bmcweb application handles the SIGHUP signal and reloads the SSL context with
  the installed certificate.
- Bmcweb invokes the Callback method with the status of the "Install" method
  received from the Certificate Manager.
- Callback method set the response message with error details for failure, sets
  the response message with newly created certificate details for success.
- Certificate object D-Bus path mapped to corresponding Redfish certificate URI.
  e.g: /xyz/openbmc_project/certs/server/https/1 is mapped to
  /redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1
  ID of the certificate is appended to the collection URI.

#### Certificate Replace
- Certificate Object implements "xyz.openbmc_project.Certs.Replace" interface to
  for replacing existing certificate.
- Redfish issues Replace certificate request by invoking the ReplaceCertificate
  action of the CertificateService.
- Redfish Certificate Collection URI is mapped to corresponding Certificate
  D-Bus object URI
  e.g: HTTPS certificate object 1 URI
  /redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1 is mapped to
  /xyz/openbmc_project/certs/server/https/1.
- Bmcweb receives POST request for Replace Certificate, invokes the Replace
  D-Bus method of the Certificate object asynchronously.
- Callback method will be passed to the bmcweb asynchronous method which will
  called after completion of the D-Bus Replace method.
- Callback method checks the response received, if failure response message is
  set with error details, if success response message is set with the replaced
  certificate details.

#### Bootup
- During bootup certificate objects created for the existing certificates.
### Errors thrown by Certificate Manager
- NotAllowed exception thrown if Install method invoked with a certificate
  already existing. At present only one certificate per server and client
  certificate type is allowed.
- InvalidCertificate excption thrown for validation errors.

#### Certificate Deletion
- For server and client certificate type the certificate deletion is not
  allowed. In case of authority certificate type the delete option is
  acceptable and can be done on individial certificates, for example:

  ```
  url: redfish/v1/Managers/bmc/Truststore/Certificates/1
  Method: DELETE

  Returns: code 204 with empty body content.
  ```

