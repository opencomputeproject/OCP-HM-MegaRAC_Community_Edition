#!/usr/bin/env python
"""
Usage: upload_and_update.py <--file tarball>
                            <--tftp user@tftp-ip:/path/to/tftproot>
                            [--password SSH-PASSWORD-TO-TFTP]
                            --bmc <bmc-ip>
                            [--noprepare]
                            [-v]

This scripts copies OpenBMC tarball to TFTP server,
and uses REST APIs to update the tarball to BMC.

Note on tftp server the tarball will be renamed to tarball_<user>
"""

import argparse
import json
import os
import subprocess
from subprocess import check_call, CalledProcessError
from time import sleep


def get_tftp_ip(tftp):
    if '@' in tftp:
        ip = tftp.split('@')[1].split(':')[0]
    else:
        ip = tftp.split(':')[0]
    return ip


def get_filename(tarball):
    return os.path.basename(tarball)

def get_server_filename(tftp, tarball):
    if '@' in tftp:
        user = tftp.split('@')[0]
    else:
        import getpass
        user = getpass.getuser()
    return get_filename(tarball) + "_" + user


def checkBmcAlive(bmc):
    cmds = ['ping', '-c', '1', bmc]
    try:
        check_call(cmds, stdout=FNULL, stderr=FNULL)
    except CalledProcessError:
        return False
    else:
        return True


def login(bmc):
    url = 'https://%s/login' % bmc
    cmds = ['curl', '-s', '-c', 'cjar', '-k', '-X', 'POST', '-H',
            'Content-Type: application/json', '-d',
            '{"data": [ "root", "0penBmc"]}', url]
    try:
        check_call(cmds, stdout=FNULL, stderr=FNULL)
    except CalledProcessError:
        return False
    else:
        return True


def prepare(bmc):
    url = 'https://%s/org/openbmc/control/flash/bmc/action/prepareForUpdate' % bmc
    cmds = ['curl', '-s', '-b', 'cjar', '-k', '-X', 'POST', '-H',
            'Content-Type: application/json', '-d',
            '{"data": []}', url]
    check_call(cmds, stdout=FNULL, stderr=FNULL)


def preserveNetwork(bmc):
    url = 'https://%s/org/openbmc/control/flash/bmc/attr/preserve_network_settings' % bmc
    cmds = ['curl', '-s', '-b', 'cjar', '-k', '-X', 'PUT', '-H',
            'Content-Type: application/json', '-d',
            '{"data": 1}', url]
    check_call(cmds, stdout=FNULL, stderr=FNULL)


def updateViaTFTP(tftp, tarball, bmc):
    tftp_ip = get_tftp_ip(tftp)
    serverfile = get_server_filename(tftp, tarball)
    url = 'https://%s/org/openbmc/control/flash/bmc/action/updateViaTftp' % bmc
    cmds = ['curl', '-s', '-b', 'cjar', '-k', '-X', 'POST', '-H',
            'Content-Type: application/json', '-d',
            '{"data": ["%s", "%s"]}' % (tftp_ip, serverfile), url]
    check_call(cmds, stdout=FNULL, stderr=FNULL)


def applyUpdate(bmc):
    url = 'https://%s/org/openbmc/control/flash/bmc/action/Apply' % bmc
    cmds = ['curl', '-s', '-b', 'cjar', '-k', '-X', 'POST', '-H',
            'Content-Type: application/json', '-d',
            '{"data": []}', url]
    check_call(cmds, stdout=FNULL, stderr=FNULL)


def getProgress(bmc):
    url = 'https://%s/org/openbmc/control/flash/bmc/action/GetUpdateProgress' % bmc
    cmds = ['curl', '-s', '-b', 'cjar', '-k', '-X', 'POST', '-H',
            'Content-Type: application/json', '-d',
            '{"data": []}', url]
    try:
        output = subprocess.check_output(cmds, stderr=FNULL)
    except CalledProcessError as e:
        # Sometimes curl fails with timeout error, let's ignore it
        return ''
    if FNULL is None:  # Do not print log when FNULL is devnull
        print output
    return json.loads(output)['data']


def reboot(bmc):
    url = 'https://%s/xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition' % bmc
    cmds = ['curl', '-s', '-b', 'cjar', '-k', '-X', 'PUT', '-H',
            'Content-Type: application/json', '-d',
            '{"data": "xyz.openbmc_project.State.BMC.Transition.Reboot"}', url]
    check_call(cmds, stdout=FNULL, stderr=FNULL)


def waitForState(state, bmc):
    status = getProgress(bmc)
    while state not in status:
        if 'Error' in status:
            raise Exception(status)
        print 'Still waiting for status: \'%s\', current: \'%s\'' % (state, status.split('\n', 1)[0])
        sleep(5)
        status = getProgress(bmc)


def upload(tftp, password, tarball):
    target = "%s/%s" % (tftp, get_server_filename(tftp, tarball))
    print 'Uploading \'%s\' to \'%s\' ...' % (tarball, target)
    if password is None:
        cmds = ['scp', tarball, target]
    else:
        cmds = ['sshpass', '-p', password, 'scp', tarball, target]
    # print cmds
    check_call(cmds, stdout=FNULL, stderr=FNULL)


def update(tftp, tarball, bmc, noprepare):
    print 'Update...'

    if not noprepare:
        login(bmc)

        print 'Prepare BMC to update'
        prepare(bmc)

        # After prepare, BMC will reboot, let's wait for it
        print 'Waiting BMC to reboot...'
        sleep(30)
        while not checkBmcAlive(bmc):
            sleep(5)
        print 'BMC is back'

    while not login(bmc):
        print 'Login fails, retry...'
        sleep(5)

    print 'Logged in'

    print 'Preserve network...'
    preserveNetwork(bmc)

    print 'Update via TFTP...'
    updateViaTFTP(tftp, tarball, bmc)

    print 'Waiting for downloading...'
    sleep(10)
    waitForState('Image ready to apply', bmc)

    print 'Apply image...'
    applyUpdate(bmc)
    sleep(10)
    waitForState('Apply Complete', bmc)

    print 'Reboot BMC...'
    reboot(bmc)
    sleep(30)
    while not checkBmcAlive(bmc):
        sleep(5)
    pass


def main():
    parser = argparse.ArgumentParser(
        description='Upload tarball to remote TFTP server and update it on BMC')
    parser.add_argument('-f', '--file', required=True, dest='tarball',
                        help='The tarball to upload and update')
    parser.add_argument('-t', '--tftp', required=True, dest='tftp',
                        help='The TFTP address including username and full path')
    parser.add_argument('-p', '--password', dest='password',
                        help='The password of TFTP server')
    parser.add_argument('-b', '--bmc', required=True, dest='bmc',
                        help='The BMC IP address')
    parser.add_argument('-n', '--noprepare', action='store_true',
                        help='Do not invoke prepare, update directly')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print verbose log')

    args = parser.parse_args()
    args = vars(args)

    if args['tftp'] is None or args['tarball'] is None or args['bmc'] is None:
        parser.print_help()
        exit(1)
    global FNULL
    if args['verbose']:
        FNULL = None  # Print log to stdout/stderr, for debug purpose
    else:
        FNULL = open(os.devnull, 'w')  # Redirect stdout/stderr to devnull

    if checkBmcAlive(args['bmc']):
        print 'BMC is alive'
    else:
        print 'BMC is down, check it first'
        exit(1)

    upload(args['tftp'], args['password'], args['tarball'])
    update(args['tftp'], args['tarball'], args['bmc'], args['noprepare'])

    print 'Completed!'

if __name__ == "__main__":
    main()
