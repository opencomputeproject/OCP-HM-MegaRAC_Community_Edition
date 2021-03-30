## To Build
```
To build this package, do the following steps:

    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS}
    3. make

To full clean the repository again run `./bootstrap.sh clean`.

SLPD:-This is a unicast SLP UDP server which serves the following
two messages
1) finsrvs
2) findsrvtypes

NOTE:- Multicast support is not there and this server neither
listen to any advertisement messages nor it advertises it's
services with DA.
