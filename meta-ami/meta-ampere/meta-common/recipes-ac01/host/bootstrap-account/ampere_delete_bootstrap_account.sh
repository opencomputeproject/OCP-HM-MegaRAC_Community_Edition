#!/bin/bash

# Initialize variables
BOOTSTRAP_ACC_LIST=$(cat /etc/passwd | awk -F: '{print $1}' | grep obmcRedfish)

for USER_NAME in ${BOOTSTRAP_ACC_LIST[@]}
do
	USER_ID=$(ipmitool user list 1 | grep $USER_NAME | cut -c1-1)
	echo "Delete Bootstrap account $USER_NAME, User ID is $USER_ID"
	ipmitool user disable $USER_ID
	ipmitool user set name $USER_ID ""
done
