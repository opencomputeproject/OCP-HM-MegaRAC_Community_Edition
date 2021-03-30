#!/bin/bash

# Starting Xcat Daemon Service
service xcatd start

# Starting SSH Daemon
echo "SSH Daemon listening on port 22"
/usr/sbin/sshd -D
