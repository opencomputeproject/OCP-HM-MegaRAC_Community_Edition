# Session Management

## Overview
IPMI RMCP+ sessions are created and maintained by phosphor-ipmi-net daemon,
whereas we need to provide details about the same using phosphor-ipmi-host.
Hence IPMI RMCP+ session details has to be exposed through D-Bus interface,
so that both phosphor-ipmi-host & phosphr-ipmi-net will be in sync.


#### xyz.openbmc_project.Ipmi.SessionInfo interface
##### properties
* SessionHandle - SessionHandle,unique one-byte number to locate the session.
* Channel   -  Session created channel.
* SessionPrivilege - Privilege of the session.
* RemoteIPAddr  – Remote IP address.
* RemotePort   - Remote port address.
* RemoteMACAddress -Remote MAC Address.
* UserID  - Session created by given user id.



#### xyz.openbmc_project.Object.Delete
#### methods
* Delete - To delete the session object in the system.

