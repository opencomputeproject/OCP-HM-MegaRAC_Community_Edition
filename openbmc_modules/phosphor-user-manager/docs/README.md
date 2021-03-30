## To Build
```
To build this package, do the following steps:

    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS}
    3. make

To clean the repository run `./bootstrap.sh clean`.
```

#### LDAP Configuration

#### Configure LDAP

```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data":[false,"ldap://<ldap://<LDAP server ip/hostname>/", "<bindDN>", "<baseDN>","<bindDNPassword>","<searchScope>","<serverType>"]}''  https://$BMC_IP/xyz/openbmc_project/user/ldap/action/CreateConfig

```
#### NOTE
If the configured ldap server is secure then we need to upload the client certificate and the CA certificate in following cases.
 - First time LDAP configuration.
 - Change the already configured Client/CA certificate

#### Upload LDAP Client Certificate

```
curl -c cjar -b cjar -k -H "Content-Type: application/octet-stream"
     -X PUT -T <FILE> https://<BMC_IP>/xyz/openbmc_project/certs/client/ldap
```

#### Upload CA Certificate

```
curl -c cjar -b cjar -k -H "Content-Type: application/octet-stream"
     -X PUT -T <FILE> https://<BMC_IP>/xyz/openbmc_project/certs/authority/ldap
```

#### Clear LDAP Config

```
curl -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data":[]}' https://$BMC_IP/xyz/openbmc_project/user/ldap/config/action/delete
```

#### Get LDAP Config

```
curl -b cjar -k https://$BMC_IP/xyz/openbmc_project/user/ldap/enumerate
```
