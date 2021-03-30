#!/usr/bin/python3
"""
 Copyright 2017,2019 IBM Corporation

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
"""
import argparse
import requests
import getpass
import json
import os
import urllib3
import time, datetime
import binascii
import subprocess
import platform
import zipfile
import tarfile
import tempfile
import hashlib
import re
import uuid
import ssl
import socket
import select
import http.client
from subprocess import check_output
import traceback


MAX_NBD_PACKET_SIZE = 131088
jsonHeader = {'Content-Type' : 'application/json'}
xAuthHeader = {}
baseTimeout = 60
serverTypeMap = {
        'ActiveDirectory' : 'active_directory',
        'OpenLDAP' : 'openldap'
        }

class NBDPipe:

    def openHTTPSocket(self,args):

        try:
            _create_unverified_https_context = ssl._create_unverified_context
        except AttributeError:
            # Legacy Python that doesn't verify HTTPS certificates by default
            pass
        else:
            # Handle target environment that doesn't support HTTPS verification
            ssl._create_default_https_context = _create_unverified_https_context


        token = gettoken(args)
        self.conn = http.client.HTTPSConnection(args.host,port=443)
        URI = "/redfish/v1/Systems/system/LogServices/SystemDump/Entries/"+str(args.dumpNum)+"/Actions/Oem/OpenBmc/LogEntry.DownloadLog"
        self.conn.request("POST",URI, headers={"X-Auth-Token":token})

    def openTCPSocket(self):
        # Create a TCP/IP socket
        self.tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # Connect the socket to the port where the server is listening
        server_address = ('localhost', 1043)
        self.tcp.connect(server_address)

    def waitformessage(self):
        inputs = [self.conn.sock,self.tcp]
        outputs = []
        message_queues = {}
        while True:
            readable, writable, exceptional = select.select(
                    inputs, outputs, inputs)

            for s in readable:
                if s is self.conn.sock:

                    data = self.conn.sock.recv(MAX_NBD_PACKET_SIZE)
                    print("<<HTTP")
                    if data:
                        self.tcp.send(data)
                    else:
                        print ("BMC Closed the connection")
                        self.conn.close()
                        self.tcp.close()
                        sys.exit(1)
                elif s is self.tcp:
                    data = self.tcp.recv(MAX_NBD_PACKET_SIZE)
                    print(">>TCP")
                    if data:
                        self.conn.sock.send(data)
                    else:
                        print("NBD server closed the connection")
                        self.conn.sock.close()
                        self.tcp.close()
                        sys.exit(1)
            for s in exceptional:
                inputs.remove(s)
                print("Exceptional closing the socket")
                s.close()

def getsize(host,args,session):
    url = "https://"+host+"/redfish/v1/Systems/system/LogServices/SystemDump/Entries/"+str(args.dumpNum)
    try:
        resp = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
        if resp.status_code==200:
            size = resp.json()["Oem"]["OpenBmc"]['SizeInB']
            return size
        else:
            return "Failed get Size"
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)

    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

def gettoken(args):
   mysess = requests.session()
   resp = mysess.post('https://'+args.host+'/login', headers=jsonHeader,json={"data":[args.user,args.PW]},verify=False)
   if resp.status_code == 200:
       cookie = resp.headers['Set-Cookie']
       match = re.search('SESSION=(\w+);', cookie)
       return match.group(1)



def get_pid(name):
    try:
        pid = map(int, check_output(["pidof", "-s",name]))
    except Exception:
        pid = 0

    return pid

def findThisProcess( process_name ):
  ps     = subprocess.Popen("ps -eaf | grep "+process_name, shell=True, stdout=subprocess.PIPE)
  output = ps.stdout.read()
  ps.stdout.close()
  ps.wait()
  pid = get_pid(process_name)
  return output

def isThisProcessRunning( process_name ):
  pid = get_pid(process_name)
  if (pid == 0 ):
    return False
  else:
    return True

def NBDSetup(host,args,session):
    user=os.getenv("SUDO_USER")
    if user is None:
        path = os.getcwd()
        nbdServerPath = path + "/nbd-server"
        if not os.path.exists(nbdServerPath):
            print("Error: this program did not run as sudo!\nplease copy nbd-server to  current directory and run script again")
            exit()

    if isThisProcessRunning('nbd-server') == True:
        print("nbd-server already Running! killing the nbd-server")
        os.system('killall nbd-server')

    if (args.dumpSaveLoc is not None):
        if(os.path.exists(args.dumpSaveLoc)):
            print("Error: File already exists.")
            exit()

    fp= open(args.dumpSaveLoc,"w")
    sizeInBytes = getsize(host,args,session)
    #Round off size to mutiples of 1024
    size = int(sizeInBytes)
    mod = size % 1024
    if mod :
        roundoff = 1024 - mod
        size = size + roundoff

    cmd = 'chmod 777 ' + args.dumpSaveLoc
    os.system(cmd)

    #Run truncate to create file with given size
    cmd = 'truncate -s ' + str(size) + ' '+ args.dumpSaveLoc
    os.system(cmd)

    if user is None:
        cmd = './nbd-server 1043 '+ args.dumpSaveLoc
    else:
        cmd = 'nbd-server 1043 '+ args.dumpSaveLoc
    os.system(cmd)


def hilight(textToColor, color, bold):
    """
         Used to add highlights to various text for displaying in a terminal

         @param textToColor: string, the text to be colored
         @param color: string, used to color the text red or green
         @param bold: boolean, used to bold the textToColor
         @return: Buffered reader containing the modified string.
    """
    if(sys.platform.__contains__("win")):
        if(color == "red"):
            os.system('color 04')
        elif(color == "green"):
            os.system('color 02')
        else:
            os.system('color') #reset to default
        return textToColor
    else:
        attr = []
        if(color == "red"):
            attr.append('31')
        elif(color == "green"):
            attr.append('32')
        else:
            attr.append('0')
        if bold:
            attr.append('1')
        else:
            attr.append('0')
        return '\x1b[%sm%s\x1b[0m' % (';'.join(attr),textToColor)

def connectionErrHandler(jsonFormat, errorStr, err):
    """
         Error handler various connection errors to bmcs

         @param jsonFormat: boolean, used to output in json format with an error code.
         @param errorStr: string, used to color the text red or green
         @param err: string, the text from the exception
    """
    if errorStr == "Timeout":
        if not jsonFormat:
            return("FQPSPIN0000M: Connection timed out. Ensure you have network connectivity to the bmc")
        else:
            conerror = {}
            conerror['CommonEventID'] = 'FQPSPIN0000M'
            conerror['sensor']="N/A"
            conerror['state']="N/A"
            conerror['additionalDetails'] = "N/A"
            conerror['Message']="Connection timed out. Ensure you have network connectivity to the BMC"
            conerror['LengthyDescription'] = "While trying to establish a connection with the specified BMC, the BMC failed to respond in adequate time. Verify the BMC is functioning properly, and the network connectivity to the BMC is stable."
            conerror['Serviceable']="Yes"
            conerror['CallHomeCandidate']= "No"
            conerror['Severity'] = "Critical"
            conerror['EventType'] = "Communication Failure/Timeout"
            conerror['VMMigrationFlag'] = "Yes"
            conerror["AffectedSubsystem"] = "Interconnect (Networking)"
            conerror["timestamp"] = str(int(time.time()))
            conerror["UserAction"] = "Verify network connectivity between the two systems and the bmc is functional."
            eventdict = {}
            eventdict['event0'] = conerror
            eventdict['numAlerts'] = '1'
            errorMessageStr = errorMessageStr = json.dumps(eventdict, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False)
            return(errorMessageStr)
    elif errorStr == "ConnectionError":
        if not jsonFormat:
            return("FQPSPIN0001M: " + str(err))
        else:
            conerror = {}
            conerror['CommonEventID'] = 'FQPSPIN0001M'
            conerror['sensor']="N/A"
            conerror['state']="N/A"
            conerror['additionalDetails'] = str(err)
            conerror['Message']="Connection Error. View additional details for more information"
            conerror['LengthyDescription'] = "A connection error to the specified BMC occurred and additional details are provided. Review these details to resolve the issue."
            conerror['Serviceable']="Yes"
            conerror['CallHomeCandidate']= "No"
            conerror['Severity'] = "Critical"
            conerror['EventType'] = "Communication Failure/Timeout"
            conerror['VMMigrationFlag'] = "Yes"
            conerror["AffectedSubsystem"] = "Interconnect (Networking)"
            conerror["timestamp"] = str(int(time.time()))
            conerror["UserAction"] = "Correct the issue highlighted in additional details and try again"
            eventdict = {}
            eventdict['event0'] = conerror
            eventdict['numAlerts'] = '1'
            errorMessageStr = json.dumps(eventdict, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False)
            return(errorMessageStr)

    else:
        return("Unknown Error: "+ str(err))


def setColWidth(keylist, numCols, dictForOutput, colNames):
    """
         Sets the output width of the columns to display

         @param keylist: list, list of strings representing the keys for the dictForOutput
         @param numcols: the total number of columns in the final output
         @param dictForOutput: dictionary, contains the information to print to the screen
         @param colNames: list, The strings to use for the column headings, in order of the keylist
         @return: A list of the column widths for each respective column.
    """
    colWidths = []
    for x in range(0, numCols):
        colWidths.append(0)
    for key in dictForOutput:
        for x in range(0, numCols):
            colWidths[x] = max(colWidths[x], len(str(dictForOutput[key][keylist[x]])))

    for x in range(0, numCols):
        colWidths[x] = max(colWidths[x], len(colNames[x])) +2

    return colWidths

def loadPolicyTable(pathToPolicyTable):
    """
         loads a json based policy table into a dictionary

         @param value: boolean, the value to convert
         @return: A string of "Yes" or "No"
    """
    policyTable = {}
    if(os.path.exists(pathToPolicyTable)):
        with open(pathToPolicyTable, 'r') as stream:
            try:
                contents =json.load(stream)
                policyTable = contents['events']
            except Exception as err:
                print(err)
    return policyTable


def boolToString(value):
    """
         converts a boolean value to a human readable string value

         @param value: boolean, the value to convert
         @return: A string of "Yes" or "No"
    """
    if(value):
        return "Yes"
    else:
        return "No"

def stringToInt(text):
    """
        returns an integer if the string can be converted, otherwise returns the string

        @param text: the string to try to convert to an integer
    """
    if text.isdigit():
        return int(text)
    else:
        return text

def naturalSort(text):
    """
        provides a way to naturally sort a list

        @param text: the key to convert for sorting
        @return list containing the broken up string parts by integers and strings
    """
    stringPartList = []
    for c in re.split('(\d+)', text):
        stringPartList.append(stringToInt(c))
    return stringPartList

def tableDisplay(keylist, colNames, output):
    """
         Logs into the BMC and creates a session

         @param keylist: list, keys for the output dictionary, ordered by colNames
         @param colNames: Names for the Table of the columns
         @param output: The dictionary of data to display
         @return: Session object
    """
    colWidth = setColWidth(keylist, len(colNames), output, colNames)
    row = ""
    outputText = ""
    for i in range(len(colNames)):
        if (i != 0): row = row + "| "
        row = row + colNames[i].ljust(colWidth[i])
    outputText += row + "\n"

    output_keys = list(output.keys())
    output_keys.sort(key=naturalSort)
    for key in output_keys:
        row = ""
        for i in range(len(keylist)):
            if (i != 0): row = row + "| "
            row = row + output[key][keylist[i]].ljust(colWidth[i])
        outputText += row + "\n"

    return outputText

def checkFWactivation(host, args, session):
    """
        Checks the software inventory for an image that is being activated.

        @return: True if an image is being activated, false is no activations are happening
    """
    url="https://"+host+"/xyz/openbmc_project/software/enumerate"
    try:
        resp = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        print(connectionErrHandler(args.json, "Timeout", None))
        return(True)
    except(requests.exceptions.ConnectionError) as err:
        print( connectionErrHandler(args.json, "ConnectionError", err))
        return True
    fwInfo = resp.json()['data']
    for key in fwInfo:
        if 'Activation' in fwInfo[key]:
            if 'Activating' in fwInfo[key]['Activation'] or 'Activating' in fwInfo[key]['RequestedActivation']:
                return True
    return False

def login(host, username, pw,jsonFormat, allowExpiredPassword):
    """
         Logs into the BMC and creates a session

         @param host: string, the hostname or IP address of the bmc to log into
         @param username: The user name for the bmc to log into
         @param pw: The password for the BMC to log into
         @param jsonFormat: boolean, flag that will only allow relevant data from user command to be display. This function becomes silent when set to true.
         @param allowExpiredPassword: true, if the requested operation should
                be allowed when the password is expired
         @return: Session object
    """
    if(jsonFormat==False):
        print("Attempting login...")
    mysess = requests.session()
    try:
        r = mysess.post('https://'+host+'/login', headers=jsonHeader, json = {"data": [username, pw]}, verify=False, timeout=baseTimeout)
        if r.status_code == 200:
            cookie = r.headers['Set-Cookie']
            match = re.search('SESSION=(\w+);', cookie)
            if match:
                xAuthHeader['X-Auth-Token'] = match.group(1)
                jsonHeader.update(xAuthHeader)
            loginMessage = json.loads(r.text)
            if (loginMessage['status'] != "ok"):
                print(loginMessage["data"]["description"].encode('utf-8'))
                sys.exit(1)
            if (('extendedMessage' in r.json()) and
                ('The password for this account must be changed' in r.json()['extendedMessage'])):
                if not allowExpiredPassword:
                    print("The password for this system has expired and must be changed"+
                            "\nsee openbmctool.py set_password --help")
                    logout(host, username, pw, mysess, jsonFormat)
                    sys.exit(1)
#         if(sys.version_info < (3,0)):
#             urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
#         if sys.version_info >= (3,0):
#             requests.packages.urllib3.disable_warnings(requests.packages.urllib3.exceptions.InsecureRequestWarning)
            return mysess
        else:
            return None
    except(requests.exceptions.Timeout):
        return (connectionErrHandler(jsonFormat, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return (connectionErrHandler(jsonFormat, "ConnectionError", err))


def logout(host, username, pw, session, jsonFormat):
    """
         Logs out of the bmc and terminates the session

         @param host: string, the hostname or IP address of the bmc to log out of
         @param username: The user name for the bmc to log out of
         @param pw: The password for the BMC to log out of
         @param session: the active session to use
         @param jsonFormat: boolean, flag that will only allow relevant data from user command to be display. This function becomes silent when set to true.
    """
    try:
        r = session.post('https://'+host+'/logout', headers=jsonHeader,json = {"data": [username, pw]}, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        print(connectionErrHandler(jsonFormat, "Timeout", None))

    if(jsonFormat==False):
        if r.status_code == 200:
            print('User ' +username + ' has been logged out')


def fru(host, args, session):
    """
         prints out the system inventory. deprecated see fruPrint and fruList

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    #url="https://"+host+"/org/openbmc/inventory/system/chassis/enumerate"

    #print(url)
    #res = session.get(url, headers=httpHeader, verify=False)
    #print(res.text)
    #sample = res.text

    #inv_list = json.loads(sample)["data"]

    url="https://"+host+"/xyz/openbmc_project/inventory/enumerate"
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))

    sample = res.text
#     inv_list.update(json.loads(sample)["data"])
#
#     #determine column width's
#     colNames = ["FRU Name", "FRU Type", "Has Fault", "Is FRU", "Present", "Version"]
#     colWidths = setColWidth(["FRU Name", "fru_type", "fault", "is_fru", "present", "version"], 6, inv_list, colNames)
#
#     print("FRU Name".ljust(colWidths[0])+ "FRU Type".ljust(colWidths[1]) + "Has Fault".ljust(colWidths[2]) + "Is FRU".ljust(colWidths[3])+
#           "Present".ljust(colWidths[4]) + "Version".ljust(colWidths[5]))
#     format the output
#     for key in sorted(inv_list.keys()):
#         keyParts = key.split("/")
#         isFRU = "True" if (inv_list[key]["is_fru"]==1) else "False"
#
#         fruEntry = (keyParts[len(keyParts) - 1].ljust(colWidths[0]) + inv_list[key]["fru_type"].ljust(colWidths[1])+
#                inv_list[key]["fault"].ljust(colWidths[2])+isFRU.ljust(colWidths[3])+
#                inv_list[key]["present"].ljust(colWidths[4])+ inv_list[key]["version"].ljust(colWidths[5]))
#         if(isTTY):
#             if(inv_list[key]["is_fru"] == 1):
#                 color = "green"
#                 bold = True
#             else:
#                 color='black'
#                 bold = False
#             fruEntry = hilight(fruEntry, color, bold)
#         print (fruEntry)
    return sample

def fruPrint(host, args, session):
    """
         prints out all inventory

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
         @return returns the total fru list.
    """
    url="https://"+host+"/xyz/openbmc_project/inventory/enumerate"
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))

    frulist={}
#     print(res.text)
    if res.status_code==200:
        frulist['Hardware'] = res.json()['data']
    else:
        if not args.json:
            return "Error retrieving the system inventory. BMC message: {msg}".format(msg=res.json()['message'])
        else:
            return res.json()
    url="https://"+host+"/xyz/openbmc_project/software/enumerate"
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
#     print(res.text)
    if res.status_code==200:
        frulist['Software'] = res.json()['data']
    else:
        if not args.json():
            return "Error retrieving the system inventory. BMC message: {msg}".format(msg=res.json()['message'])
        else:
            return res.json()
    return frulist


def fruList(host, args, session):
    """
         prints out all inventory or only a specific specified item

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if(args.items==True):
        return fruPrint(host, args, session)
    else:
        return fruPrint(host, args, session)



def fruStatus(host, args, session):
    """
         prints out the status of all FRUs

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    url="https://"+host+"/xyz/openbmc_project/inventory/enumerate"
    try:
        res = session.get(url, headers=jsonHeader, verify=False)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
#     print(res.text)
    frulist = res.json()['data']
    frus = {}
    for key in frulist:
        component = frulist[key]
        isFru = False
        present = False
        func = False
        hasSels = False
        keyPieces = key.split('/')
        fruName = keyPieces[-1]
        if 'core' in fruName: #associate cores to cpus
            fruName = keyPieces[-2] + '-' + keyPieces[-1]
        if 'Functional' in component:
            if('Present' in component):
                if 'FieldReplaceable' in component:
                    if component['FieldReplaceable'] == 1:
                        isFru = True
                if "fan" in fruName:
                    isFru = True;
                if component['Present'] == 1:
                    present = True
                if component['Functional'] == 1:
                    func = True
                if ((key + "/fault") in frulist):
                    hasSels = True;
                if args.verbose:
                    if hasSels:
                        loglist = []
                        faults = frulist[key+"/fault"]['endpoints']
                        for item in faults:
                            loglist.append(item.split('/')[-1])
                        frus[fruName] = {"compName": fruName, "Functional": boolToString(func), "Present":boolToString(present), "IsFru": boolToString(isFru), "selList": ', '.join(loglist).strip() }
                    else:
                        frus[fruName] = {"compName": fruName, "Functional": boolToString(func), "Present":boolToString(present), "IsFru": boolToString(isFru), "selList": "None" }
                else:
                    frus[fruName] = {"compName": fruName, "Functional": boolToString(func), "Present":boolToString(present), "IsFru": boolToString(isFru), "hasSEL": boolToString(hasSels) }
        elif "power_supply" in fruName or "powersupply" in fruName:
            if component['Present'] ==1:
                present = True
            isFru = True
            if ((key + "/fault") in frulist):
                hasSels = True;
            if args.verbose:
                if hasSels:
                    loglist = []
                    faults = frulist[key+"/fault"]['endpoints']
                    for item in faults:
                        loglist.append(item.split('/')[-1])
                    frus[fruName] = {"compName": fruName, "Functional": "No", "Present":boolToString(present), "IsFru": boolToString(isFru), "selList": ', '.join(loglist).strip() }
                else:
                    frus[fruName] = {"compName": fruName, "Functional": "Yes", "Present":boolToString(present), "IsFru": boolToString(isFru), "selList": "None" }
            else:
                frus[fruName] = {"compName": fruName, "Functional": boolToString(not hasSels), "Present":boolToString(present), "IsFru": boolToString(isFru), "hasSEL": boolToString(hasSels) }
    if not args.json:
        if not args.verbose:
            colNames = ["Component", "Is a FRU", "Present", "Functional", "Has Logs"]
            keylist = ["compName", "IsFru", "Present", "Functional", "hasSEL"]
        else:
            colNames = ["Component", "Is a FRU", "Present", "Functional", "Assoc. Log Number(s)"]
            keylist = ["compName", "IsFru", "Present", "Functional", "selList"]
        return tableDisplay(keylist, colNames, frus)
    else:
        return str(json.dumps(frus, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False))

def sensor(host, args, session):
    """
         prints out all sensors

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the sensor sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    url="https://"+host+"/xyz/openbmc_project/sensors/enumerate"
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))

    #Get OCC status
    url="https://"+host+"/org/open_power/control/enumerate"
    try:
        occres = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    if not args.json:
        colNames = ['sensor', 'type', 'units', 'value', 'target']
        sensors = res.json()["data"]
        output = {}
        for key in sensors:
            senDict = {}
            keyparts = key.split("/")

            # Associations like the following also show up here:
            # /xyz/openbmc_project/sensors/<type>/<name>/<assoc-name>
            # Skip them.
            # Note:  keyparts[0] = '' which is why there are 7 segments.
            if len(keyparts) > 6:
                continue

            senDict['sensorName'] = keyparts[-1]
            senDict['type'] = keyparts[-2]
            try:
                senDict['units'] = sensors[key]['Unit'].split('.')[-1]
            except KeyError:
                senDict['units'] = "N/A"
            if('Scale' in sensors[key]):
                scale = 10 ** sensors[key]['Scale']
            else:
                scale = 1
            try:
                senDict['value'] = str(sensors[key]['Value'] * scale)
            except KeyError:
                if 'value' in sensors[key]:
                    senDict['value'] = sensors[key]['value']
                else:
                    senDict['value'] = "N/A"
            if 'Target' in sensors[key]:
                senDict['target'] = str(sensors[key]['Target'])
            else:
                senDict['target'] = 'N/A'
            output[senDict['sensorName']] = senDict

        occstatus = occres.json()["data"]
        if '/org/open_power/control/occ0' in occstatus:
            occ0 = occstatus["/org/open_power/control/occ0"]['OccActive']
            if occ0 == 1:
                occ0 = 'Active'
            else:
                occ0 = 'Inactive'
            output['OCC0'] = {'sensorName':'OCC0', 'type': 'Discrete', 'units': 'N/A', 'value': occ0, 'target': 'Active'}
            occ1 = occstatus["/org/open_power/control/occ1"]['OccActive']
            if occ1 == 1:
                occ1 = 'Active'
            else:
                occ1 = 'Inactive'
            output['OCC1'] = {'sensorName':'OCC1', 'type': 'Discrete', 'units': 'N/A', 'value': occ0, 'target': 'Active'}
        else:
            output['OCC0'] = {'sensorName':'OCC0', 'type': 'Discrete', 'units': 'N/A', 'value': 'Inactive', 'target': 'Inactive'}
            output['OCC1'] = {'sensorName':'OCC1', 'type': 'Discrete', 'units': 'N/A', 'value': 'Inactive', 'target': 'Inactive'}
        keylist = ['sensorName', 'type', 'units', 'value', 'target']

        return tableDisplay(keylist, colNames, output)
    else:
        return res.text + occres.text

def sel(host, args, session):
    """
         prints out the bmc alerts

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the sel sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """

    url="https://"+host+"/xyz/openbmc_project/logging/entry/enumerate"
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    return res.text


def parseESEL(args, eselRAW):
    """
         parses the esel data and gets predetermined search terms

         @param eselRAW: string, the raw esel string from the bmc
         @return: A dictionary containing the quick snapshot data unless args.fullEsel is listed then a full PEL log is returned
    """
    eselParts = {}
    esel_bin = binascii.unhexlify(''.join(eselRAW.split()[16:]))
    #search terms contains the search term as the key and the return dictionary key as it's value
    searchTerms = { 'Signature Description':'signatureDescription', 'devdesc':'devdesc',
                    'Callout type': 'calloutType', 'Procedure':'procedure', 'Sensor Type': 'sensorType'}
    uniqueID = str(uuid.uuid4())
    eselBinPath = tempfile.gettempdir() + os.sep + uniqueID + 'esel.bin'
    with open(eselBinPath, 'wb') as f:
        f.write(esel_bin)
    errlPath = ""
    #use the right errl file for the machine architecture
    arch = platform.machine()
    if(arch =='x86_64' or arch =='AMD64'):
        if os.path.exists('/opt/ibm/ras/bin/x86_64/errl'):
            errlPath = '/opt/ibm/ras/bin/x86_64/errl'
        elif os.path.exists('errl/x86_64/errl'):
            errlPath = 'errl/x86_64/errl'
        else:
            errlPath = 'x86_64/errl'
    elif (platform.machine()=='ppc64le'):
        if os.path.exists('/opt/ibm/ras/bin/ppc64le/errl'):
            errlPath = '/opt/ibm/ras/bin/ppc64le/errl'
        elif os.path.exists('errl/ppc64le/errl'):
            errlPath = 'errl/ppc64le/errl'
        else:
            errlPath = 'ppc64le/errl'
    else:
        print("machine architecture not supported for parsing eSELs")
        return eselParts

    if(os.path.exists(errlPath)):
        output= subprocess.check_output([errlPath, '-d', '--file='+eselBinPath]).decode('utf-8')
#         output = proc.communicate()[0]
        lines = output.split('\n')

        if(hasattr(args, 'fullEsel')):
            return output

        for i in range(0, len(lines)):
            lineParts = lines[i].split(':')
            if(len(lineParts)>1): #ignore multi lines, output formatting lines, and other information
                for term in searchTerms:
                    if(term in lineParts[0]):
                        temp = lines[i][lines[i].find(':')+1:].strip()[:-1].strip()
                        if lines[i+1].find(':') != -1:
                            if (len(lines[i+1].split(':')[0][1:].strip())==0):
                                while(len(lines[i][:lines[i].find(':')].strip())>2):
                                    #has multiple lines, process and update line counter
                                    if((i+1) <= len(lines)):
                                        i+=1
                                    else:
                                        i=i-1
                                        break
                                    #Append the content from the next line removing the pretty display characters
                                    #Finds the first colon then starts 2 characters after, then removes all whitespace
                                    temp = temp + lines[i][lines[i].find(':')+2:].strip()[:-1].strip()[:-1].strip()
                        if(searchTerms[term] in eselParts):
                            eselParts[searchTerms[term]] = eselParts[searchTerms[term]] + ", " + temp
                        else:
                            eselParts[searchTerms[term]] = temp
        os.remove(eselBinPath)
    else:
        print("errl file cannot be found")

    return eselParts


def getESELSeverity(esel):
    """
        Finds the severity type in an eSEL from the User Header section.
        @param esel - the eSEL data
        @return severity - e.g. 'Critical'
    """

    # everything but 1 and 2 are Critical
    # '1': 'recovered',
    # '2': 'predictive',
    # '4': 'unrecoverable',
    # '5': 'critical',
    # '6': 'diagnostic',
    # '7': 'symptom'
    severities = {
        '1': 'Informational',
        '2': 'Warning'
    }

    try:
        headerPosition = esel.index('55 48') # 'UH'
        # The severity is the last byte in the 8 byte section (a byte is '  bb')
        severity = esel[headerPosition:headerPosition+32].split(' ')[-1]
        type = severity[0]
    except ValueError:
        print("Could not find severity value in UH section in eSEL")
        type = 'x';

    return severities.get(type, 'Critical')


def sortSELs(events):
    """
         sorts the sels by timestamp, then log entry number

         @param events: Dictionary containing events
         @return: list containing a list of the ordered log entries, and dictionary of keys
    """
    logNumList = []
    timestampList = []
    eventKeyDict = {}
    eventsWithTimestamp = {}
    logNum2events = {}
    for key in events:
        if key == 'numAlerts': continue
        if 'callout' in key: continue
        timestamp = (events[key]['timestamp'])
        if timestamp not in timestampList:
            eventsWithTimestamp[timestamp] = [events[key]['logNum']]
        else:
            eventsWithTimestamp[timestamp].append(events[key]['logNum'])
        #map logNumbers to the event dictionary keys
        eventKeyDict[str(events[key]['logNum'])] = key

    timestampList = list(eventsWithTimestamp.keys())
    timestampList.sort()
    for ts in timestampList:
        if len(eventsWithTimestamp[ts]) > 1:
            tmplist = eventsWithTimestamp[ts]
            tmplist.sort()
            logNumList = logNumList + tmplist
        else:
            logNumList = logNumList + eventsWithTimestamp[ts]

    return [logNumList, eventKeyDict]


def parseAlerts(policyTable, selEntries, args):
    """
         parses alerts in the IBM CER format, using an IBM policy Table

         @param policyTable: dictionary, the policy table entries
         @param selEntries: dictionary, the alerts retrieved from the bmc
         @return: A dictionary of the parsed entries, in chronological order
    """
    eventDict = {}
    eventNum =""
    count = 0
    esel = ""
    eselParts = {}
    i2cdevice= ""
    eselSeverity = None

    'prepare and sort the event entries'
    sels = {}
    for key in selEntries:
        if '/xyz/openbmc_project/logging/entry/' not in key: continue
        if 'callout' not in key:
            sels[key] = selEntries[key]
            sels[key]['logNum'] = key.split('/')[-1]
            sels[key]['timestamp'] = selEntries[key]['Timestamp']
    sortedEntries = sortSELs(sels)
    logNumList = sortedEntries[0]
    eventKeyDict = sortedEntries[1]

    for logNum in logNumList:
        key = eventKeyDict[logNum]
        hasEsel=False
        i2creadFail = False
        if 'callout' in key:
            continue
        else:
            messageID = str(selEntries[key]['Message'])
            addDataPiece = selEntries[key]['AdditionalData']
            calloutIndex = 0
            calloutFound = False
            for i in range(len(addDataPiece)):
                if("CALLOUT_INVENTORY_PATH" in addDataPiece[i]):
                    calloutIndex = i
                    calloutFound = True
                    fruCallout = str(addDataPiece[calloutIndex]).split('=')[1]
                if("CALLOUT_DEVICE_PATH" in addDataPiece[i]):
                    i2creadFail = True

                    fruCallout = str(addDataPiece[calloutIndex]).split('=')[1]

                    # Fall back to "I2C"/"FSI" if dev path isn't in policy table
                    if (messageID + '||' + fruCallout) not in policyTable:
                        i2cdevice = str(addDataPiece[i]).strip().split('=')[1]
                        i2cdevice = '/'.join(i2cdevice.split('/')[-4:])
                        if 'fsi' in str(addDataPiece[calloutIndex]).split('=')[1]:
                            fruCallout = 'FSI'
                        else:
                            fruCallout = 'I2C'
                    calloutFound = True
                if("CALLOUT_GPIO_NUM" in addDataPiece[i]):
                    if not calloutFound:
                        fruCallout = 'GPIO'
                    calloutFound = True
                if("CALLOUT_IIC_BUS" in addDataPiece[i]):
                    if not calloutFound:
                        fruCallout = "I2C"
                    calloutFound = True
                if("CALLOUT_IPMI_SENSOR_NUM" in addDataPiece[i]):
                    if not calloutFound:
                        fruCallout = "IPMI"
                    calloutFound = True
                if("ESEL" in addDataPiece[i]):
                    esel = str(addDataPiece[i]).strip().split('=')[1]
                    eselSeverity = getESELSeverity(esel)
                    if args.devdebug:
                        eselParts = parseESEL(args, esel)
                    hasEsel=True
                if("GPU" in addDataPiece[i]):
                    fruCallout = '/xyz/openbmc_project/inventory/system/chassis/motherboard/gpu' + str(addDataPiece[i]).strip()[-1]
                    calloutFound = True
                if("PROCEDURE" in addDataPiece[i]):
                    fruCallout = str(hex(int(str(addDataPiece[i]).split('=')[1])))[2:]
                    calloutFound = True
                if("RAIL_NAME" in addDataPiece[i]):
                    calloutFound=True
                    fruCallout = str(addDataPiece[i]).split('=')[1].strip()
                if("INPUT_NAME" in addDataPiece[i]):
                    calloutFound=True
                    fruCallout = str(addDataPiece[i]).split('=')[1].strip()
                if("SENSOR_TYPE" in addDataPiece[i]):
                    calloutFound=True
                    fruCallout = str(addDataPiece[i]).split('=')[1].strip()

            if(calloutFound):
                if fruCallout.strip() != "":
                    policyKey = messageID +"||" +  fruCallout

                    # Also use the severity for hostboot errors
                    if eselSeverity and messageID == 'org.open_power.Host.Error.Event':
                        policyKey += '||' + eselSeverity

                        # if not in the table, fall back to the original key
                        if policyKey not in policyTable:
                            policyKey = policyKey.replace('||'+eselSeverity, '')

                    if policyKey not in policyTable:
                        policyKey = messageID
                else:
                    policyKey = messageID
            else:
                policyKey = messageID
            event = {}
            eventNum = str(count)
            if policyKey in policyTable:
                for pkey in policyTable[policyKey]:
                    if(type(policyTable[policyKey][pkey])== bool):
                        event[pkey] = boolToString(policyTable[policyKey][pkey])
                    else:
                        if (i2creadFail and pkey == 'Message'):
                            event[pkey] = policyTable[policyKey][pkey] + ' ' +i2cdevice
                        else:
                            event[pkey] = policyTable[policyKey][pkey]
                event['timestamp'] = selEntries[key]['Timestamp']
                event['resolved'] = bool(selEntries[key]['Resolved'])
                if(hasEsel):
                    if args.devdebug:
                        event['eselParts'] = eselParts
                    event['raweSEL'] = esel
                event['logNum'] = key.split('/')[-1]
                eventDict['event' + eventNum] = event

            else:
                severity = str(selEntries[key]['Severity']).split('.')[-1]
                if severity == 'Error':
                    severity = 'Critical'
                eventDict['event'+eventNum] = {}
                eventDict['event' + eventNum]['error'] = "error: Not found in policy table: " + policyKey
                eventDict['event' + eventNum]['timestamp'] = selEntries[key]['Timestamp']
                eventDict['event' + eventNum]['Severity'] = severity
                if(hasEsel):
                    if args.devdebug:
                        eventDict['event' +eventNum]['eselParts'] = eselParts
                    eventDict['event' +eventNum]['raweSEL'] = esel
                eventDict['event' +eventNum]['logNum'] = key.split('/')[-1]
                eventDict['event' +eventNum]['resolved'] = bool(selEntries[key]['Resolved'])
            count += 1
    return eventDict


def selDisplay(events, args):
    """
         displays alerts in human readable format

         @param events: Dictionary containing events
         @return:
    """
    activeAlerts = []
    historyAlerts = []
    sortedEntries = sortSELs(events)
    logNumList = sortedEntries[0]
    eventKeyDict = sortedEntries[1]
    keylist = ['Entry', 'ID', 'Timestamp', 'Serviceable', 'Severity','Message']
    if(args.devdebug):
        colNames = ['Entry', 'ID', 'Timestamp', 'Serviceable', 'Severity','Message',  'eSEL contents']
        keylist.append('eSEL')
    else:
        colNames = ['Entry', 'ID', 'Timestamp', 'Serviceable', 'Severity', 'Message']
    for log in logNumList:
        selDict = {}
        alert = events[eventKeyDict[str(log)]]
        if('error' in alert):
            selDict['Entry'] = alert['logNum']
            selDict['ID'] = 'Unknown'
            selDict['Timestamp'] = datetime.datetime.fromtimestamp(int(alert['timestamp']/1000)).strftime("%Y-%m-%d %H:%M:%S")
            msg = alert['error']
            polMsg = msg.split("policy table:")[0]
            msg = msg.split("policy table:")[1]
            msgPieces = msg.split("||")
            err = msgPieces[0]
            if(err.find("org.open_power.")!=-1):
                err = err.split("org.open_power.")[1]
            elif(err.find("xyz.openbmc_project.")!=-1):
                err = err.split("xyz.openbmc_project.")[1]
            else:
                err = msgPieces[0]
            callout = ""
            if len(msgPieces) >1:
                callout = msgPieces[1]
                if(callout.find("/org/open_power/")!=-1):
                    callout = callout.split("/org/open_power/")[1]
                elif(callout.find("/xyz/openbmc_project/")!=-1):
                    callout = callout.split("/xyz/openbmc_project/")[1]
                else:
                    callout = msgPieces[1]
            selDict['Message'] = polMsg +"policy table: "+ err +  "||" + callout
            selDict['Serviceable'] = 'Unknown'
            selDict['Severity'] = alert['Severity']
        else:
            selDict['Entry'] = alert['logNum']
            selDict['ID'] = alert['CommonEventID']
            selDict['Timestamp'] = datetime.datetime.fromtimestamp(int(alert['timestamp']/1000)).strftime("%Y-%m-%d %H:%M:%S")
            selDict['Message'] = alert['Message']
            selDict['Serviceable'] = alert['Serviceable']
            selDict['Severity'] = alert['Severity']


        eselOrder = ['refCode','signatureDescription', 'eselType', 'devdesc', 'calloutType', 'procedure']
        if ('eselParts' in alert and args.devdebug):
            eselOutput = ""
            for item in eselOrder:
                if item in alert['eselParts']:
                    eselOutput = eselOutput + item + ": " + alert['eselParts'][item] + " | "
            selDict['eSEL'] = eselOutput
        else:
            if args.devdebug:
                selDict['eSEL'] = "None"

        if not alert['resolved']:
            activeAlerts.append(selDict)
        else:
            historyAlerts.append(selDict)
    mergedOutput = activeAlerts + historyAlerts
    colWidth = setColWidth(keylist, len(colNames), dict(enumerate(mergedOutput)), colNames)

    output = ""
    if(len(activeAlerts)>0):
        row = ""
        output +="----Active Alerts----\n"
        for i in range(0, len(colNames)):
            if i!=0: row =row + "| "
            row = row + colNames[i].ljust(colWidth[i])
        output += row + "\n"

        for i in range(0,len(activeAlerts)):
            row = ""
            for j in range(len(activeAlerts[i])):
                if (j != 0): row = row + "| "
                row = row + activeAlerts[i][keylist[j]].ljust(colWidth[j])
            output += row + "\n"

    if(len(historyAlerts)>0):
        row = ""
        output+= "----Historical Alerts----\n"
        for i in range(len(colNames)):
            if i!=0: row =row + "| "
            row = row + colNames[i].ljust(colWidth[i])
        output += row + "\n"

        for i in range(0, len(historyAlerts)):
            row = ""
            for j in range(len(historyAlerts[i])):
                if (j != 0): row = row + "| "
                row = row + historyAlerts[i][keylist[j]].ljust(colWidth[j])
            output += row + "\n"
#         print(events[eventKeyDict[str(log)]])
    return output


def selPrint(host, args, session):
    """
         prints out all bmc alerts

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if(args.policyTableLoc is None):
        if os.path.exists('policyTable.json'):
            ptableLoc = "policyTable.json"
        elif os.path.exists('/opt/ibm/ras/lib/policyTable.json'):
            ptableLoc = '/opt/ibm/ras/lib/policyTable.json'
        else:
            ptableLoc = 'lib/policyTable.json'
    else:
        ptableLoc = args.policyTableLoc
    policyTable = loadPolicyTable(ptableLoc)
    rawselEntries = ""
    if(hasattr(args, 'fileloc') and args.fileloc is not None):
        if os.path.exists(args.fileloc):
            with open(args.fileloc, 'r') as selFile:
                selLines = selFile.readlines()
            rawselEntries = ''.join(selLines)
        else:
            print("Error: File not found")
            sys.exit(1)
    else:
        rawselEntries = sel(host, args, session)
    loadFailed = False
    try:
        selEntries = json.loads(rawselEntries)
    except ValueError:
        loadFailed = True
    if loadFailed:
        cleanSels = json.dumps(rawselEntries).replace('\\n', '')
        #need to load json twice as original content was string escaped a second time
        selEntries = json.loads(json.loads(cleanSels))
    selEntries = selEntries['data']

    if 'description' in selEntries:
        if(args.json):
            return("{\n\t\"numAlerts\": 0\n}")
        else:
            return("No log entries found")

    else:
        if(len(policyTable)>0):
            events = parseAlerts(policyTable, selEntries, args)
            if(args.json):
                events["numAlerts"] = len(events)
                retValue = str(json.dumps(events, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False))
                return retValue
            elif(hasattr(args, 'fullSel')):
                return events
            else:
                #get log numbers to order event entries sequentially
                return selDisplay(events, args)
        else:
            if(args.json):
                return selEntries
            else:
                print("error: Policy Table not found.")
                return selEntries

def selList(host, args, session):
    """
         prints out all all bmc alerts, or only prints out the specified alerts

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    return(sel(host, args, session))


def selClear(host, args, session):
    """
         clears all alerts

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    url="https://"+host+"/xyz/openbmc_project/logging/action/DeleteAll"
    data = "{\"data\": [] }"

    try:
        res = session.post(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    if res.status_code == 200:
        return "The Alert Log has been cleared. Please allow a few minutes for the action to complete."
    else:
        print("Unable to clear the logs, trying to clear 1 at a time")
        sels = json.loads(sel(host, args, session))['data']
        for key in sels:
            if 'callout' not in key:
                logNum = key.split('/')[-1]
                url = "https://"+ host+ "/xyz/openbmc_project/logging/entry/"+logNum+"/action/Delete"
                try:
                    session.post(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
                except(requests.exceptions.Timeout):
                    return connectionErrHandler(args.json, "Timeout", None)
                    sys.exit(1)
                except(requests.exceptions.ConnectionError) as err:
                    return connectionErrHandler(args.json, "ConnectionError", err)
                    sys.exit(1)
        return ('Sel clearing complete')

def selSetResolved(host, args, session):
    """
         sets a sel entry to resolved

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    url="https://"+host+"/xyz/openbmc_project/logging/entry/" + str(args.selNum) + "/attr/Resolved"
    data = "{\"data\": 1 }"
    try:
        res = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    if res.status_code == 200:
        return "Sel entry "+ str(args.selNum) +" is now set to resolved"
    else:
        return "Unable to set the alert to resolved"

def selResolveAll(host, args, session):
    """
         sets a sel entry to resolved

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    rawselEntries = sel(host, args, session)
    loadFailed = False
    try:
        selEntries = json.loads(rawselEntries)
    except ValueError:
        loadFailed = True
    if loadFailed:
        cleanSels = json.dumps(rawselEntries).replace('\\n', '')
        #need to load json twice as original content was string escaped a second time
        selEntries = json.loads(json.loads(cleanSels))
    selEntries = selEntries['data']

    if 'description' in selEntries:
        if(args.json):
            return("{\n\t\"selsResolved\": 0\n}")
        else:
            return("No log entries found")
    else:
        d = vars(args)
        successlist = []
        failedlist = []
        for key in selEntries:
            if 'callout' not in key:
                d['selNum'] = key.split('/')[-1]
                resolved = selSetResolved(host,args,session)
                if 'Sel entry' in resolved:
                    successlist.append(d['selNum'])
                else:
                    failedlist.append(d['selNum'])
        output = ""
        successlist.sort()
        failedlist.sort()
        if len(successlist)>0:
            output = "Successfully resolved: " +', '.join(successlist) +"\n"
        if len(failedlist)>0:
            output += "Failed to resolve: " + ', '.join(failedlist) + "\n"
        return output

def chassisPower(host, args, session):
    """
         called by the chassis function. Controls the power state of the chassis, or gets the status

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if(args.powcmd == 'on'):
        if checkFWactivation(host, args, session):
            return ("Chassis Power control disabled during firmware activation")
        print("Attempting to Power on...:")
        url="https://"+host+"/xyz/openbmc_project/state/host0/attr/RequestedHostTransition"
        data = '{"data":"xyz.openbmc_project.State.Host.Transition.On"}'
        try:
            res = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        return res.text
    elif(args.powcmd == 'softoff'):
        if checkFWactivation(host, args, session):
            return ("Chassis Power control disabled during firmware activation")
        print("Attempting to Power off gracefully...:")
        url="https://"+host+"/xyz/openbmc_project/state/host0/attr/RequestedHostTransition"
        data = '{"data":"xyz.openbmc_project.State.Host.Transition.Off"}'
        try:
            res = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        return res.text
    elif(args.powcmd == 'hardoff'):
        if checkFWactivation(host, args, session):
            return ("Chassis Power control disabled during firmware activation")
        print("Attempting to Power off immediately...:")
        url="https://"+host+"/xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition"
        data = '{"data":"xyz.openbmc_project.State.Chassis.Transition.Off"}'
        try:
            res = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        return res.text
    elif(args.powcmd == 'status'):
        url="https://"+host+"/xyz/openbmc_project/state/chassis0/attr/CurrentPowerState"
        try:
            res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        chassisState = json.loads(res.text)['data'].split('.')[-1]
        url="https://"+host+"/xyz/openbmc_project/state/host0/attr/CurrentHostState"
        try:
            res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        hostState = json.loads(res.text)['data'].split('.')[-1]
        url="https://"+host+"/xyz/openbmc_project/state/bmc0/attr/CurrentBMCState"
        try:
            res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        bmcState = json.loads(res.text)['data'].split('.')[-1]
        if(args.json):
            outDict = {"Chassis Power State" : chassisState, "Host Power State" : hostState, "BMC Power State":bmcState}
            return json.dumps(outDict, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False)
        else:
            return "Chassis Power State: " +chassisState + "\nHost Power State: " + hostState + "\nBMC Power State: " + bmcState
    else:
        return "Invalid chassis power command"


def chassisIdent(host, args, session):
    """
         called by the chassis function. Controls the identify led of the chassis. Sets or gets the state

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if(args.identcmd == 'on'):
        print("Attempting to turn identify light on...:")
        url="https://"+host+"/xyz/openbmc_project/led/groups/enclosure_identify/attr/Asserted"
        data = '{"data":true}'
        try:
            res = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        return res.text
    elif(args.identcmd == 'off'):
        print("Attempting to turn identify light off...:")
        url="https://"+host+"/xyz/openbmc_project/led/groups/enclosure_identify/attr/Asserted"
        data = '{"data":false}'
        try:
            res = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        return res.text
    elif(args.identcmd == 'status'):
        url="https://"+host+"/xyz/openbmc_project/led/groups/enclosure_identify"
        try:
            res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        status = json.loads(res.text)['data']
        if(args.json):
            return status
        else:
            if status['Asserted'] == 0:
                return "Identify light is off"
            else:
                return "Identify light is blinking"
    else:
        return "Invalid chassis identify command"


def chassis(host, args, session):
    """
         controls the different chassis commands

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fru sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if(hasattr(args, 'powcmd')):
        result = chassisPower(host,args,session)
    elif(hasattr(args, 'identcmd')):
        result = chassisIdent(host, args, session)
    else:
        return "This feature is not yet implemented"
    return result

def dumpRetrieve(host, args, session):
    """
         Downloads dump of given dump type

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    dumpType = args.dumpType
    if (args.dumpType=="SystemDump"):
        dumpResp=systemDumpRetrieve(host,args,session)
    elif(args.dumpType=="bmc"):
        dumpResp=bmcDumpRetrieve(host,args,session)
    return dumpResp

def dumpList(host, args, session):
    """
         Lists dump of the given dump type

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if (args.dumpType=="SystemDump"):
        dumpResp=systemDumpList(host,args,session)
    elif(args.dumpType=="bmc"):
        dumpResp=bmcDumpList(host,args,session)
    return dumpResp

def dumpDelete(host, args, session):
    """
         Deletes dump of the given dump type

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if (args.dumpType=="SystemDump"):
        dumpResp=systemDumpDelete(host,args,session)
    elif(args.dumpType=="bmc"):
        dumpResp=bmcDumpDelete(host,args,session)
    return dumpResp

def dumpDeleteAll(host, args, session):
    """
         Deletes all dumps of the given dump type

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if (args.dumpType=="SystemDump"):
        dumpResp=systemDumpDeleteAll(host,args,session)
    elif(args.dumpType=="bmc"):
        dumpResp=bmcDumpDeleteAll(host,args,session)
    return dumpResp

def dumpCreate(host, args, session):
    """
         Creates dump for the given dump type

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if (args.dumpType=="SystemDump"):
        dumpResp=systemDumpCreate(host,args,session)
    elif(args.dumpType=="bmc"):
        dumpResp=bmcDumpCreate(host,args,session)
    return dumpResp


def bmcDumpRetrieve(host, args, session):
    """
         Downloads a dump file from the bmc

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    dumpNum = args.dumpNum
    if (args.dumpSaveLoc is not None):
        saveLoc = args.dumpSaveLoc
    else:
        saveLoc = tempfile.gettempdir()
    url ='https://'+host+'/download/dump/' + str(dumpNum)
    try:
        r = session.get(url, headers=jsonHeader, stream=True, verify=False, timeout=baseTimeout)
        if (args.dumpSaveLoc is not None):
            if os.path.exists(saveLoc):
                if saveLoc[-1] != os.path.sep:
                    saveLoc = saveLoc + os.path.sep
                filename = saveLoc + host+'-dump' + str(dumpNum) + '.tar.xz'

            else:
                return 'Invalid save location specified'
        else:
            filename = tempfile.gettempdir()+os.sep + host+'-dump' + str(dumpNum) + '.tar.xz'

        with open(filename, 'wb') as f:
                    for chunk in r.iter_content(chunk_size =1024):
                        if chunk:
                            f.write(chunk)
        return 'Saved as ' + filename

    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)

    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

def bmcDumpList(host, args, session):
    """
         Lists the number of dump files on the bmc

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    url ='https://'+host+'/xyz/openbmc_project/dump/list'
    try:
        r = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
        dumpList = r.json()
        formattedList = []
        #remove items that aren't dump entries 'entry, internal, manager endpoints'
        if 'data' in dumpList:
            for entry in dumpList['data']:
                if 'entry' in entry:
                    if entry.split('/')[-1].isnumeric():
                        formattedList.append(entry)
            dumpList['data']= formattedList
        return dumpList
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)

    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

def bmcDumpDelete(host, args, session):
    """
         Deletes BMC dump files from the bmc

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    dumpList = []
    successList = []
    failedList = []
    if args.dumpNum is not None:
        if isinstance(args.dumpNum, list):
            dumpList = args.dumpNum
        else:
            dumpList.append(args.dumpNum)
        for dumpNum in dumpList:
            url ='https://'+host+'/xyz/openbmc_project/dump/entry/'+str(dumpNum)+'/action/Delete'
            try:
                r = session.post(url, headers=jsonHeader, json = {"data": []}, verify=False, timeout=baseTimeout)
                if r.status_code == 200:
                    successList.append(str(dumpNum))
                else:
                    failedList.append(str(dumpNum))
            except(requests.exceptions.Timeout):
                return connectionErrHandler(args.json, "Timeout", None)
            except(requests.exceptions.ConnectionError) as err:
                return connectionErrHandler(args.json, "ConnectionError", err)
        output = "Successfully deleted dumps: " + ', '.join(successList)
        if(len(failedList)>0):
            output+= '\nFailed to delete dumps: ' + ', '.join(failedList)
        return output
    else:
        return 'You must specify an entry number to delete'

def bmcDumpDeleteAll(host, args, session):
    """
         Deletes All BMC dump files from the bmc

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    dumpResp = bmcDumpList(host, args, session)
    if 'FQPSPIN0000M' in dumpResp or 'FQPSPIN0001M'in dumpResp:
        return dumpResp
    dumpList = dumpResp['data']
    d = vars(args)
    dumpNums = []
    for dump in dumpList:
        dumpNum = dump.strip().split('/')[-1]
        if dumpNum.isdigit():
            dumpNums.append(int(dumpNum))
    d['dumpNum'] = dumpNums

    return bmcDumpDelete(host, args, session)


def bmcDumpCreate(host, args, session):
    """
         Creates a bmc dump file

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    url = 'https://'+host+'/xyz/openbmc_project/dump/action/CreateDump'
    try:
        r = session.post(url, headers=jsonHeader, json = {"data": []}, verify=False, timeout=baseTimeout)
        info = r.json()
        if(r.status_code == 200 and not args.json):
            return ('Dump successfully created')
        elif(args.json):
            return info
        elif 'data' in info:
            if 'QuotaExceeded' in info['data']['description']:
                return 'BMC dump space is full. Please delete at least one existing dump entry and try again.'
            else:
                return "Failed to create a BMC dump. BMC Response:\n {resp}".format(resp=info)
        else:
            return "Failed to create a BMC dump. BMC Response:\n {resp}".format(resp=info)
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)


def systemDumpRetrieve(host, args, session):
    """
         Downloads system dump

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    NBDSetup(host,args,session)
    pipe = NBDPipe()
    pipe.openHTTPSocket(args)
    pipe.openTCPSocket()
    pipe.waitformessage()

def systemDumpList(host, args, session):
    """
         Lists system dumps

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    print("in systemDumpList")
    url = "https://"+host+"/redfish/v1/Systems/system/LogServices/"+args.dumpType+"/Entries"
    try:
        r = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
        dumpList = r.json()
        return dumpList
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)

    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)


def systemDumpDelete(host, args, session):
    """
         Deletes system dump

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    dumpList = []
    successList = []
    failedList = []
    if args.dumpNum is not None:
        if isinstance(args.dumpNum, list):
            dumpList = args.dumpNum
        else:
            dumpList.append(args.dumpNum)
        for dumpNum in dumpList:
            url = 'https://'+host+'/redfish/v1/Systems/system/LogServices/'+args.dumpType+'/Entries/'+ str(dumpNum)
            try:
                r = session.delete(url, headers=jsonHeader, json = {"data": []}, verify=False, timeout=baseTimeout)
                if r.status_code == 200:
                    successList.append(str(dumpNum))
                else:
                    failedList.append(str(dumpNum))
            except(requests.exceptions.Timeout):
                return connectionErrHandler(args.json, "Timeout", None)
            except(requests.exceptions.ConnectionError) as err:
                return connectionErrHandler(args.json, "ConnectionError", err)
        output = "Successfully deleted dumps: " + ', '.join(successList)
        if(len(failedList)>0):
            output+= '\nFailed to delete dumps: ' + ', '.join(failedList)
        return output
    else:
        return 'You must specify an entry number to delete'

def systemDumpDeleteAll(host, args, session):
    """
         Deletes All system dumps

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    url = 'https://'+host+'/redfish/v1/Systems/system/LogServices/'+args.dumpType+'/Actions/LogService.ClearLog'
    try:
        r = session.post(url, headers=jsonHeader, json = {"data": []}, verify=False, timeout=baseTimeout)
        if(r.status_code == 200 and not args.json):
            return ('Dumps successfully cleared')
        elif(args.json):
            return r.json()
        else:
            return ('Failed to clear dumps')
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

def systemDumpCreate(host, args, session):
    """
         Creates a system dump

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    url =  'https://'+host+'/redfish/v1/Systems/system/LogServices/'+args.dumpType+'/Actions/Oem/Openbmc/LogService.CreateLog'
    try:
        r = session.post(url, headers=jsonHeader, json = {"data": []}, verify=False, timeout=baseTimeout)
        if(r.status_code == 200 and not args.json):
            return ('Dump successfully created')
        elif(args.json):
            return r.json()
        else:
            return ('Failed to create dump')
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

def csdDumpInitiate(host, args, session):
    """
        Starts the process of getting the current list of dumps then initiates the creation of one.

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    errorInfo = ""
    dumpcount = 0
    try:
        d = vars(args)
        d['json'] = True
    except Exception as e:
        errorInfo += "Failed to set the json flag to True \n Exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    try:
        for i in range(3):
            dumpInfo = bmcDumpList(host, args, session)
            if 'data' in dumpInfo:
                dumpcount = len(dumpInfo['data'])
                break
            else:
                errorInfo+= "Dump List Message returned: " + json.dumps(dumpInfo,indent=0, separators=(',', ':')).replace('\n','') +"\n"
    except Exception as e:
        errorInfo+= "Failed to collect the list of dumps.\nException: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    #Create a user initiated dump
    dumpFailure = True
    try:
        for i in range(3):
            dumpcreated = bmcDumpCreate(host, args, session)
            if 'message' in dumpcreated:
                if 'ok' in dumpcreated['message'].lower():
                    dumpFailure = False
                    break
                elif 'data' in dumpcreated:
                    if 'QuotaExceeded' in dumpcreated['data']['description']:
                        print('Not enough dump space on the BMC to create a new dump. Please delete the oldest entry (lowest number) and rerun the collect_service_data command.')
                        errorInfo+='Dump Space is full. No new dump was created with this collection'
                        break
                    else:
                        errorInfo+= "Dump create message returned: " + json.dumps(dumpcreated,indent=0, separators=(',', ':')).replace('\n','') +"\n"
                else:
                    errorInfo+= "Dump create message returned: " + json.dumps(dumpcreated,indent=0, separators=(',', ':')).replace('\n','') +"\n"
            else:
                errorInfo+= "Dump create message returned: " + json.dumps(dumpcreated,indent=0, separators=(',', ':')).replace('\n','') +"\n"
    except Exception as e:
        errorInfo+= "Dump create exception encountered: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    output = {}
    output['errors'] = errorInfo
    output['dumpcount'] = dumpcount
    if dumpFailure: output['dumpFailure'] = True
    return output

def csdInventory(host, args,session, fileDir):
    """
        Collects the BMC inventory, retrying if necessary

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
         @param fileDir: string representation of the path to use for putting files created
    """
    errorInfo = "===========Inventory =============\n"
    output={}
    inventoryCollected = False
    try:
        for i in range(3):
            frulist = fruPrint(host, args, session)
            if 'Hardware' in frulist:
                inventoryCollected = True
                break
            else:
                errorInfo += json.dumps(frulist, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False) + '\n'
    except Exception as e:
        errorInfo += "Inventory collection exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()
    if inventoryCollected:
        try:
            with open(fileDir +os.sep+'inventory.txt', 'w') as f:
                f.write(json.dumps(frulist, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False) + '\n')
            print("Inventory collected and stored in " + fileDir + os.sep + "inventory.txt")
            output['fileLoc'] = fileDir+os.sep+'inventory.txt'
        except Exception as e:
            print("Failed to write inventory to file.")
            errorInfo += "Error writing inventory to the file. Exception: {eInfo}\n".format(eInfo=e)
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
            errorInfo += traceback.format_exc()

    output['errors'] = errorInfo

    return output

def csdSensors(host, args,session, fileDir):
    """
        Collects the BMC sensor readings, retrying if necessary

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
         @param fileDir: string representation of the path to use for putting files created
    """
    errorInfo = "===========Sensors =============\n"
    sensorsCollected = False
    output={}
    try:
        d = vars(args)
        d['json'] = False
    except Exception as e:
        errorInfo += "Failed to set the json flag to False \n Exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    try:
        for i in range(3):
            sensorReadings = sensor(host, args, session)
            if 'OCC0' in sensorReadings:
                sensorsCollected = True
                break
            else:
                errorInfo += sensorReadings
    except Exception as e:
        errorInfo += "Sensor reading collection exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()
    if sensorsCollected:
        try:
            with open(fileDir +os.sep+'sensorReadings.txt', 'w') as f:
                f.write(sensorReadings)
            print("Sensor readings collected and stored in " + fileDir + os.sep+ "sensorReadings.txt")
            output['fileLoc'] = fileDir+os.sep+'sensorReadings.txt'
        except Exception as e:
            print("Failed to write sensor readings to file system.")
            errorInfo += "Error writing sensor readings to the file. Exception: {eInfo}\n".format(eInfo=e)
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
            errorInfo += traceback.format_exc()

    output['errors'] = errorInfo
    return output

def csdLEDs(host,args, session, fileDir):
    """
        Collects the BMC LED status, retrying if necessary

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
         @param fileDir: string representation of the path to use for putting files created
    """
    errorInfo = "===========LEDs =============\n"
    ledsCollected = False
    output={}
    try:
        d = vars(args)
        d['json'] = True
    except Exception as e:
        errorInfo += "Failed to set the json flag to False \n Exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()
    try:
        url="https://"+host+"/xyz/openbmc_project/led/enumerate"
        httpHeader = {'Content-Type':'application/json'}
        for i in range(3):
            try:
                ledRes = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
                if ledRes.status_code == 200:
                    ledsCollected = True
                    leds = ledRes.json()['data']
                    break
                else:
                    errorInfo += ledRes.text
            except(requests.exceptions.Timeout):
                errorInfo+=json.dumps( connectionErrHandler(args.json, "Timeout", None), sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False) + '\n'
            except(requests.exceptions.ConnectionError) as err:
                errorInfo += json.dumps(connectionErrHandler(args.json, "ConnectionError", err), sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False) + '\n'
                exc_type, exc_obj, exc_tb = sys.exc_info()
                fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
                errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
                errorInfo += traceback.format_exc()
    except Exception as e:
        errorInfo += "LED status collection exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    if ledsCollected:
        try:
            with open(fileDir +os.sep+'ledStatus.txt', 'w') as f:
                f.write(json.dumps(leds, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False) + '\n')
            print("LED status collected and stored in " + fileDir + os.sep+ "ledStatus.txt")
            output['fileLoc'] = fileDir+os.sep+'ledStatus.txt'
        except Exception as e:
            print("Failed to write LED status to file system.")
            errorInfo += "Error writing LED status to the file. Exception: {eInfo}\n".format(eInfo=e)
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
            errorInfo += traceback.format_exc()

    output['errors'] = errorInfo
    return output

def csdSelShortList(host, args, session, fileDir):
    """
        Collects the BMC log entries, retrying if necessary

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
         @param fileDir: string representation of the path to use for putting files created
    """
    errorInfo = "===========SEL Short List =============\n"
    selsCollected = False
    output={}
    try:
        d = vars(args)
        d['json'] = False
    except Exception as e:
        errorInfo += "Failed to set the json flag to False \n Exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    try:
        for i in range(3):
            sels = selPrint(host,args,session)
            if '----Active Alerts----' in sels or 'No log entries found' in sels or '----Historical Alerts----' in sels:
                selsCollected = True
                break
            else:
                errorInfo += sels + '\n'
    except Exception as e:
        errorInfo += "SEL short list collection exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    if selsCollected:
        try:
            with open(fileDir +os.sep+'SELshortlist.txt', 'w') as f:
                f.write(sels)
            print("SEL short list collected and stored in " + fileDir + os.sep+ "SELshortlist.txt")
            output['fileLoc'] = fileDir+os.sep+'SELshortlist.txt'
        except Exception as e:
            print("Failed to write SEL short list to file system.")
            errorInfo += "Error writing SEL short list to the file. Exception: {eInfo}\n".format(eInfo=e)
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
            errorInfo += traceback.format_exc()

    output['errors'] = errorInfo
    return output

def csdParsedSels(host, args, session, fileDir):
    """
        Collects the BMC log entries, retrying if necessary

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
         @param fileDir: string representation of the path to use for putting files created
    """
    errorInfo = "===========SEL Parsed List =============\n"
    selsCollected = False
    output={}
    try:
        d = vars(args)
        d['json'] = True
        d['fullEsel'] = True
    except Exception as e:
        errorInfo += "Failed to set the json flag to True \n Exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    try:
        for i in range(3):
            parsedfullsels = json.loads(selPrint(host,args,session))
            if 'numAlerts' in parsedfullsels:
                selsCollected = True
                break
            else:
                errorInfo += parsedfullsels + '\n'
    except Exception as e:
        errorInfo += "Parsed full SELs collection exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    if selsCollected:
        try:
            sortedSELs = sortSELs(parsedfullsels)
            with open(fileDir +os.sep+'parsedSELs.txt', 'w') as f:
                for log in sortedSELs[0]:
                    esel = ""
                    parsedfullsels[sortedSELs[1][str(log)]]['timestamp'] = datetime.datetime.fromtimestamp(int(parsedfullsels[sortedSELs[1][str(log)]]['timestamp']/1000)).strftime("%Y-%m-%d %H:%M:%S")
                    if ('raweSEL' in parsedfullsels[sortedSELs[1][str(log)]] and args.devdebug):
                        esel = parsedfullsels[sortedSELs[1][str(log)]]['raweSEL']
                        del parsedfullsels[sortedSELs[1][str(log)]]['raweSEL']
                    f.write(json.dumps(parsedfullsels[sortedSELs[1][str(log)]],sort_keys=True, indent=4, separators=(',', ': ')))
                    if(args.devdebug and esel != ""):
                        f.write(parseESEL(args, esel))
            print("Parsed SELs collected and stored in " + fileDir + os.sep+ "parsedSELs.txt")
            output['fileLoc'] = fileDir+os.sep+'parsedSELs.txt'
        except Exception as e:
            print("Failed to write fully parsed SELs to file system.")
            errorInfo += "Error writing fully parsed SELs to the file. Exception: {eInfo}\n".format(eInfo=e)
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
            errorInfo += traceback.format_exc()

    output['errors'] = errorInfo
    return output

def csdFullEnumeration(host, args, session, fileDir):
    """
        Collects a full enumeration of /xyz/openbmc_project/, retrying if necessary

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
         @param fileDir: string representation of the path to use for putting files created
    """
    errorInfo = "===========BMC Full Enumeration =============\n"
    bmcFullCollected = False
    output={}
    try:
        d = vars(args)
        d['json'] = True
    except Exception as e:
        errorInfo += "Failed to set the json flag to False \n Exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()
    try:
        print("Attempting to get a full BMC enumeration")
        url="https://"+host+"/xyz/openbmc_project/enumerate"
        httpHeader = {'Content-Type':'application/json'}
        for i in range(3):
            try:
                bmcRes = session.get(url, headers=jsonHeader, verify=False, timeout=180)
                if bmcRes.status_code == 200:
                    bmcFullCollected = True
                    fullEnumeration = bmcRes.json()
                    break
                else:
                    errorInfo += bmcRes.text
            except(requests.exceptions.Timeout):
                errorInfo+=json.dumps( connectionErrHandler(args.json, "Timeout", None), sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False) + '\n'
            except(requests.exceptions.ConnectionError) as err:
                errorInfo += json.dumps(connectionErrHandler(args.json, "ConnectionError", err), sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False) + '\n'
                exc_type, exc_obj, exc_tb = sys.exc_info()
                fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
                errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
                errorInfo += traceback.format_exc()
    except Exception as e:
        errorInfo += "RAW BMC data collection exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    if bmcFullCollected:
        try:
            with open(fileDir +os.sep+'bmcFullRaw.txt', 'w') as f:
                f.write(json.dumps(fullEnumeration, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False) + '\n')
            print("RAW BMC data collected and saved into " + fileDir + os.sep+ "bmcFullRaw.txt")
            output['fileLoc'] = fileDir+os.sep+'bmcFullRaw.txt'
        except Exception as e:
            print("Failed to write RAW BMC data  to file system.")
            errorInfo += "Error writing RAW BMC data collection to the file. Exception: {eInfo}\n".format(eInfo=e)
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
            errorInfo += traceback.format_exc()

    output['errors'] = errorInfo
    return output

def csdCollectAllDumps(host, args, session, fileDir):
    """
        Collects all of the bmc dump files and stores them in fileDir

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the collectServiceData sub command
        @param session: the active session to use
        @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
        @param fileDir: string representation of the path to use for putting files created
    """

    errorInfo = "===========BMC Dump Collection =============\n"
    dumpListCollected = False
    output={}
    dumpList = {}
    try:
        d = vars(args)
        d['json'] = True
        d['dumpSaveLoc'] = fileDir
    except Exception as e:
        errorInfo += "Failed to set the json flag to True, or failed to set the dumpSave Location \n Exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    print('Collecting bmc dump files')

    try:
        for i in range(3):
            dumpResp = bmcDumpList(host, args, session)
            if 'message' in dumpResp:
                if 'ok' in dumpResp['message'].lower():
                    dumpList = dumpResp['data']
                    dumpListCollected = True
                    break
                else:
                    errorInfo += "Status was not OK when retrieving the list of dumps available. \n Response: \n{resp}\n".format(resp=dumpResp)
            else:
                errorInfo += "Invalid response received from the BMC while retrieving the list of dumps available.\n {resp}\n".format(resp=dumpResp)
    except Exception as e:
        errorInfo += "BMC dump list exception: {eInfo}\n".format(eInfo=e)
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()

    if dumpListCollected:
        output['fileList'] = []
        for dump in dumpList:
            try:
                if '/xyz/openbmc_project/dump/internal/manager' not in dump:
                    d['dumpNum'] = int(dump.strip().split('/')[-1])
                    print('retrieving dump file ' + str(d['dumpNum']))
                    filename = bmcDumpRetrieve(host, args, session).split('Saved as ')[-1]
                    output['fileList'].append(filename)
            except Exception as e:
                print("Unable to collect dump: {dumpInfo}".format(dumpInfo=dump))
                errorInfo += "Exception collecting a bmc dump {dumpInfo}\n {eInfo}\n".format(dumpInfo=dump, eInfo=e)
                exc_type, exc_obj, exc_tb = sys.exc_info()
                fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
                errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
                errorInfo += traceback.format_exc()
    output['errors'] = errorInfo
    return output

def collectServiceData(host, args, session):
    """
         Collects all data needed for service from the BMC

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the collectServiceData sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """

    global toolVersion
    filelist = []
    errorInfo = ""

    #get current number of bmc dumps and create a new bmc dump
    dumpInitdata = csdDumpInitiate(host, args, session)
    if 'dumpFailure' in dumpInitdata:
        return 'Collect service data is stopping due to not being able to create a new dump. No service data was collected.'
    dumpcount = dumpInitdata['dumpcount']
    errorInfo += dumpInitdata['errors']
    #create the directory to put files
    try:
        args.silent = True
        myDir = tempfile.gettempdir()+os.sep + host + "--" + datetime.datetime.now().strftime("%Y-%m-%d_%H.%M.%S")
        os.makedirs(myDir)

    except Exception as e:
        print('Unable to create the temporary directory for data collection. Ensure sufficient privileges to create temporary directory. Aborting.')
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        errorInfo += "Exception: Error: {err}, Details: {etype}, {fname}, {lineno}\n".format(err=e, etype=exc_type, fname=fname, lineno=exc_tb.tb_lineno)
        errorInfo += traceback.format_exc()
        return("Python exception: {eInfo}".format(eInfo = e))

    #Collect Inventory
    inventoryData = csdInventory(host, args, session, myDir)
    if 'fileLoc' in inventoryData:
        filelist.append(inventoryData['fileLoc'])
    errorInfo += inventoryData['errors']
    #Read all the sensor and OCC status
    sensorData = csdSensors(host,args,session,myDir)
    if 'fileLoc' in sensorData:
        filelist.append(sensorData['fileLoc'])
    errorInfo += sensorData['errors']
    #Collect all of the LEDs status
    ledStatus = csdLEDs(host, args, session, myDir)
    if 'fileLoc' in ledStatus:
        filelist.append(ledStatus['fileLoc'])
    errorInfo += ledStatus['errors']

    #Collect the bmc logs
    selShort = csdSelShortList(host, args, session, myDir)
    if 'fileLoc' in selShort:
        filelist.append(selShort['fileLoc'])
    errorInfo += selShort['errors']

    parsedSELs = csdParsedSels(host, args, session, myDir)
    if 'fileLoc' in parsedSELs:
        filelist.append(parsedSELs['fileLoc'])
    errorInfo += parsedSELs['errors']

    #collect RAW bmc enumeration
    bmcRaw = csdFullEnumeration(host, args, session, myDir)
    if 'fileLoc' in bmcRaw:
        filelist.append(bmcRaw['fileLoc'])
    errorInfo += bmcRaw['errors']

    #wait for new dump to finish being created
    waitingForNewDump = True
    count = 0;
    print("Waiting for new BMC dump to finish being created. Wait time could be up to 5 minutes")
    while(waitingForNewDump):
        dumpList = bmcDumpList(host, args, session)['data']
        if len(dumpList) > dumpcount:
            waitingForNewDump = False
            break;
        elif(count>150):
            print("Timed out waiting for bmc to make a new dump file. Continuing without it.")
            break;
        else:
            time.sleep(2)
        count += 1

    #collect all of the dump files
    getBMCDumps = csdCollectAllDumps(host, args, session, myDir)
    if 'fileList' in getBMCDumps:
        filelist+= getBMCDumps['fileList']
    errorInfo += getBMCDumps['errors']

    #write the runtime errors to a file
    try:
        with open(myDir +os.sep+'openbmctoolRuntimeErrors.txt', 'w') as f:
            f.write(errorInfo)
        print("OpenBMC tool runtime errors collected and stored in " + myDir + os.sep+ "openbmctoolRuntimeErrors.txt")
        filelist.append(myDir+os.sep+'openbmctoolRuntimeErrors.txt')
    except Exception as e:
        print("Failed to write OpenBMC tool runtime errors to file system.")

    #create the zip file
    try:
        filename = myDir.split(tempfile.gettempdir()+os.sep)[-1] + "_" + toolVersion + '_openbmc.zip'
        zf = zipfile.ZipFile(myDir+os.sep + filename, 'w')
        for myfile in filelist:
            zf.write(myfile, os.path.basename(myfile))
        zf.close()
        print("Zip file with all collected data created and stored in: {fileInfo}".format(fileInfo=myDir+os.sep+filename))
    except Exception as e:
        print("Failed to create zip file with collected information")
    return "data collection finished"


def healthCheck(host, args, session):
    """
         runs a health check on the platform

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the bmc sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    #check fru status and get as json to easily work through
    d = vars(args)
    useJson = d['json']
    d['json'] = True
    d['verbose']= False

    frus = json.loads(fruStatus(host, args, session))

    hwStatus= "OK"
    performanceStatus = "OK"
    for key in frus:
        if frus[key]["Functional"] == "No" and frus[key]["Present"] == "Yes":
            hwStatus= "Degraded"
            if("power_supply" in key or "powersupply" in key):
                gpuCount =0
                for comp in frus:
                    if "gv100card" in comp:
                        gpuCount +=1
                if gpuCount > 4:
                    hwStatus = "Critical"
                    performanceStatus="Degraded"
                    break;
            elif("fan" in key):
                hwStatus = "Degraded"
            else:
                performanceStatus = "Degraded"
    if useJson:
        output = {"Hardware Status": hwStatus, "Performance": performanceStatus}
        output = json.dumps(output, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False)
    else:
        output = ("Hardware Status: " + hwStatus +
                  "\nPerformance: " +performanceStatus )


    #SW407886: Clear the duplicate entries
    #collect the dups
    d['devdebug'] = False
    sels = json.loads(selPrint(host, args, session))
    logNums2Clr = []
    oldestLogNum={"logNum": "bogus" ,"key" : ""}
    count = 0
    if sels['numAlerts'] > 0:
        for key in sels:
            if "numAlerts" in key:
                continue
            try:
                if "slave@00:00/00:00:00:06/sbefifo1-dev0/occ1-dev0" in sels[key]['Message']:
                    count += 1
                    if count > 1:
                        #preserve first occurrence
                        if sels[key]['timestamp'] < sels[oldestLogNum['key']]['timestamp']:
                            oldestLogNum['key']=key
                            oldestLogNum['logNum'] = sels[key]['logNum']
                    else:
                        oldestLogNum['key']=key
                        oldestLogNum['logNum'] = sels[key]['logNum']
                    logNums2Clr.append(sels[key]['logNum'])
            except KeyError:
                continue
        if(count >0):
            logNums2Clr.remove(oldestLogNum['logNum'])
        #delete the dups
        if count >1:
            data = "{\"data\": [] }"
            for logNum in logNums2Clr:
                    url = "https://"+ host+ "/xyz/openbmc_project/logging/entry/"+logNum+"/action/Delete"
                    try:
                        session.post(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
                    except(requests.exceptions.Timeout):
                        deleteFailed = True
                    except(requests.exceptions.ConnectionError) as err:
                        deleteFailed = True
    #End of defect resolve code
    d['json'] = useJson
    return output



def bmc(host, args, session):
    """
         handles various bmc level commands, currently bmc rebooting

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the bmc sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if(args.type is not None):
        return bmcReset(host, args, session)
    if(args.info):
        return "Not implemented at this time"



def bmcReset(host, args, session):
    """
         controls resetting the bmc. warm reset reboots the bmc, cold reset removes the configuration and reboots.

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the bmcReset sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    if checkFWactivation(host, args, session):
        return ("BMC reset control disabled during firmware activation")
    if(args.type == "warm"):
        print("\nAttempting to reboot the BMC...:")
        url="https://"+host+"/xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition"
        data = '{"data":"xyz.openbmc_project.State.BMC.Transition.Reboot"}'
        res = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        return res.text
    elif(args.type =="cold"):
        print("\nAttempting to reboot the BMC...:")
        url="https://"+host+"/xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition"
        data = '{"data":"xyz.openbmc_project.State.BMC.Transition.Reboot"}'
        res = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        return res.text
    else:
        return "invalid command"

def gardClear(host, args, session):
    """
         clears the gard records from the bmc

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the gardClear sub command
         @param session: the active session to use
    """
    url="https://"+host+"/org/open_power/control/gard/action/Reset"
    data = '{"data":[]}'
    try:

        res = session.post(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        if res.status_code == 404:
            return "Command not supported by this firmware version"
        else:
            return res.text
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

def activateFWImage(host, args, session):
    """
         activates a firmware image on the bmc

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fwflash sub command
         @param session: the active session to use
         @param fwID: the unique ID of the fw image to activate
    """
    fwID = args.imageID

    #determine the existing versions
    url="https://"+host+"/xyz/openbmc_project/software/enumerate"
    try:
        resp = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    existingSoftware = json.loads(resp.text)['data']
    altVersionID = ''
    versionType = ''
    imageKey = '/xyz/openbmc_project/software/'+fwID
    if imageKey in existingSoftware:
        versionType = existingSoftware[imageKey]['Purpose']
    for key in existingSoftware:
        if imageKey == key:
            continue
        if 'Purpose' in existingSoftware[key]:
            if versionType == existingSoftware[key]['Purpose']:
                altVersionID = key.split('/')[-1]




    url="https://"+host+"/xyz/openbmc_project/software/"+ fwID + "/attr/Priority"
    url1="https://"+host+"/xyz/openbmc_project/software/"+ altVersionID + "/attr/Priority"
    data = "{\"data\": 0}"
    data1 = "{\"data\": 1 }"
    try:
        resp = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        resp1 = session.put(url1, headers=jsonHeader, data=data1, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if(not args.json):
        if resp.status_code == 200 and resp1.status_code == 200:
            return 'Firmware flash and activation completed. Please reboot the bmc and then boot the host OS for the changes to take effect. '
        else:
            return "Firmware activation failed."
    else:
        return resp.text + resp1.text

def activateStatus(host, args, session):
    if checkFWactivation(host, args, session):
        return("Firmware is currently being activated. Do not reboot the BMC or start the Host OS")
    else:
        return("No firmware activations are pending")

def extractFWimage(path, imageType):
    """
         extracts the bmc image and returns information about the package

         @param path: the path and file name of the firmware image
         @param imageType: The type of image the user is trying to flash. Host or BMC
         @return: the image id associated with the package. returns an empty string on error.
    """
    f = tempfile.TemporaryFile()
    tmpDir = tempfile.gettempdir()
    newImageID = ""
    if os.path.exists(path):
        try:
            imageFile = tarfile.open(path,'r')
            contents = imageFile.getmembers()
            for tf in contents:
                if 'MANIFEST' in tf.name:
                    imageFile.extract(tf.name, path=tmpDir)
                    with open(tempfile.gettempdir() +os.sep+ tf.name, 'r') as imageInfo:
                        for line in imageInfo:
                            if 'purpose' in line:
                                purpose = line.split('=')[1]
                                if imageType not in purpose.split('.')[-1]:
                                    print('The specified image is not for ' + imageType)
                                    print('Please try again with the image for ' + imageType)
                                    return ""
                            if 'version' == line.split('=')[0]:
                                version = line.split('=')[1].strip().encode('utf-8')
                                m = hashlib.sha512()
                                m.update(version)
                                newImageID = m.hexdigest()[:8]
                                break
                    try:
                        os.remove(tempfile.gettempdir() +os.sep+ tf.name)
                    except OSError:
                        pass
                    return newImageID
        except tarfile.ExtractError as e:
            print('Unable to extract information from the firmware file.')
            print('Ensure you have write access to the directory: ' + tmpDir)
            return newImageID
        except tarfile.TarError as e:
            print('This is not a valid firmware file.')
            return newImageID
        print("This is not a valid firmware file.")
        return newImageID
    else:
        print('The filename and path provided are not valid.')
        return newImageID

def getAllFWImageIDs(fwInvDict):
    """
         gets a list of all the firmware image IDs

         @param fwInvDict: the dictionary to search for FW image IDs
         @return: list containing string representation of the found image ids
    """
    idList = []
    for key in fwInvDict:
        if 'Version' in fwInvDict[key]:
            idList.append(key.split('/')[-1])
    return idList

def fwFlash(host, args, session):
    """
         updates the bmc firmware and pnor firmware

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the fwflash sub command
         @param session: the active session to use
    """
    d = vars(args)
    if(args.type == 'bmc'):
        purp = 'BMC'
    else:
        purp = 'Host'

    #check power state of the machine. No concurrent FW updates allowed
    d['powcmd'] = 'status'
    powerstate = chassisPower(host, args, session)
    if 'Chassis Power State: On' in powerstate:
        return("Aborting firmware update. Host is powered on. Please turn off the host and try again.")

    #determine the existing images on the bmc
    url="https://"+host+"/xyz/openbmc_project/software/enumerate"
    try:
        resp = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return connectionErrHandler(args.json, "Timeout", None)
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    oldsoftware = json.loads(resp.text)['data']

    #Extract the tar and get information from the manifest file
    newversionID = extractFWimage(args.fileloc, purp)
    if  newversionID == "":
        return "Unable to verify FW image."


    #check if the new image is already on the bmc
    if newversionID not in getAllFWImageIDs(oldsoftware):

        #upload the file
        httpHeader = {'Content-Type':'application/octet-stream'}
        httpHeader.update(xAuthHeader)
        url="https://"+host+"/upload/image"
        data=open(args.fileloc,'rb').read()
        print("Uploading file to BMC")
        try:
            resp = session.post(url, headers=httpHeader, data=data, verify=False)
        except(requests.exceptions.Timeout):
            return connectionErrHandler(args.json, "Timeout", None)
        except(requests.exceptions.ConnectionError) as err:
            return connectionErrHandler(args.json, "ConnectionError", err)
        if resp.status_code != 200:
            return "Failed to upload the file to the bmc"
        else:
            print("Upload complete.")

        #verify bmc processed the image
        software ={}
        for i in range(0, 5):
            url="https://"+host+"/xyz/openbmc_project/software/enumerate"
            try:
                resp = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
            except(requests.exceptions.Timeout):
                return connectionErrHandler(args.json, "Timeout", None)
            except(requests.exceptions.ConnectionError) as err:
                return connectionErrHandler(args.json, "ConnectionError", err)
            software = json.loads(resp.text)['data']
            #check if bmc is done processing the new image
            if (newversionID in getAllFWImageIDs(software)):
                break
            else:
                time.sleep(15)

        #activate the new image
        print("Activating new image: "+newversionID)
        url="https://"+host+"/xyz/openbmc_project/software/"+ newversionID + "/attr/RequestedActivation"
        data = '{"data":"xyz.openbmc_project.Software.Activation.RequestedActivations.Active"}'
        try:
            resp = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            return connectionErrHandler(args.json, "Timeout", None)
        except(requests.exceptions.ConnectionError) as err:
            return connectionErrHandler(args.json, "ConnectionError", err)

        #wait for the activation to complete, timeout after ~1 hour
        i=0
        while i < 360:
            url="https://"+host+"/xyz/openbmc_project/software/"+ newversionID
            data = '{"data":"xyz.openbmc_project.Software.Activation.RequestedActivations.Active"}'
            try:
                resp = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
            except(requests.exceptions.Timeout):
                return connectionErrHandler(args.json, "Timeout", None)
            except(requests.exceptions.ConnectionError) as err:
                return connectionErrHandler(args.json, "ConnectionError", err)
            fwInfo = json.loads(resp.text)['data']
            if 'Activating' not in fwInfo['Activation'] and 'Activating' not in fwInfo['RequestedActivation']:
                print('')
                break
            else:
                sys.stdout.write('.')
                sys.stdout.flush()
                time.sleep(10) #check every 10 seconds
        return "Firmware flash and activation completed. Please reboot the bmc and then boot the host OS for the changes to take effect. "
    else:
        print("This image has been found on the bmc. Activating image: " + newversionID)

        d['imageID'] = newversionID
        return activateFWImage(host, args, session)

def getFWInventoryAttributes(rawFWInvItem, ID):
    """
         gets and lists all of the firmware in the system.

         @return: returns a dictionary containing the image attributes
    """
    reqActivation = rawFWInvItem["RequestedActivation"].split('.')[-1]
    pendingActivation = ""
    if reqActivation == "None":
        pendingActivation = "No"
    else:
        pendingActivation = "Yes"
    firmwareAttr = {ID: {
        "Purpose": rawFWInvItem["Purpose"].split('.')[-1],
        "Version": rawFWInvItem["Version"],
        "RequestedActivation": pendingActivation,
        "ID": ID}}

    if "ExtendedVersion" in rawFWInvItem:
        firmwareAttr[ID]['ExtendedVersion'] = rawFWInvItem['ExtendedVersion'].split(',')
    else:
        firmwareAttr[ID]['ExtendedVersion'] = ""
    return firmwareAttr

def parseFWdata(firmwareDict):
    """
         creates a dictionary with parsed firmware data

         @return: returns a dictionary containing the image attributes
    """
    firmwareInfoDict = {"Functional": {}, "Activated":{}, "NeedsActivated":{}}
    for key in firmwareDict['data']:
        #check for valid endpoint
        if "Purpose" in firmwareDict['data'][key]:
            id = key.split('/')[-1]
            if firmwareDict['data'][key]['Activation'].split('.')[-1] == "Active":
                fwActivated = True
            else:
                fwActivated = False
            if 'Priority' in firmwareDict['data'][key]:
                if firmwareDict['data'][key]['Priority'] == 0:
                    firmwareInfoDict['Functional'].update(getFWInventoryAttributes(firmwareDict['data'][key], id))
                elif firmwareDict['data'][key]['Priority'] >= 0 and fwActivated:
                    firmwareInfoDict['Activated'].update(getFWInventoryAttributes(firmwareDict['data'][key], id))
                else:
                    firmwareInfoDict['NeedsActivated'].update(getFWInventoryAttributes(firmwareDict['data'][key], id))
            else:
                firmwareInfoDict['NeedsActivated'].update(getFWInventoryAttributes(firmwareDict['data'][key], id))
    emptySections = []
    for key in firmwareInfoDict:
        if len(firmwareInfoDict[key])<=0:
            emptySections.append(key)
    for key in emptySections:
        del firmwareInfoDict[key]
    return firmwareInfoDict

def displayFWInvenory(firmwareInfoDict, args):
    """
         gets and lists all of the firmware in the system.

         @return: returns a string containing all of the firmware information
    """
    output = ""
    if not args.json:
        for key in firmwareInfoDict:
            for subkey in firmwareInfoDict[key]:
                firmwareInfoDict[key][subkey]['ExtendedVersion'] = str(firmwareInfoDict[key][subkey]['ExtendedVersion'])
        if not args.verbose:
            output = "---Running Images---\n"
            colNames = ["Purpose", "Version", "ID"]
            keylist = ["Purpose", "Version", "ID"]
            output += tableDisplay(keylist, colNames, firmwareInfoDict["Functional"])
            if "Activated" in firmwareInfoDict:
                output += "\n---Available Images---\n"
                output += tableDisplay(keylist, colNames, firmwareInfoDict["Activated"])
            if "NeedsActivated" in firmwareInfoDict:
                output += "\n---Needs Activated Images---\n"
                output += tableDisplay(keylist, colNames, firmwareInfoDict["NeedsActivated"])

        else:
            output = "---Running Images---\n"
            colNames = ["Purpose", "Version", "ID", "Pending Activation", "Extended Version"]
            keylist = ["Purpose", "Version", "ID", "RequestedActivation", "ExtendedVersion"]
            output += tableDisplay(keylist, colNames, firmwareInfoDict["Functional"])
            if "Activated" in firmwareInfoDict:
                output += "\n---Available Images---\n"
                output += tableDisplay(keylist, colNames, firmwareInfoDict["Activated"])
            if "NeedsActivated" in firmwareInfoDict:
                output += "\n---Needs Activated Images---\n"
                output += tableDisplay(keylist, colNames, firmwareInfoDict["NeedsActivated"])
        return output
    else:
        return str(json.dumps(firmwareInfoDict, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False))

def firmwareList(host, args, session):
    """
         gets and lists all of the firmware in the system.

         @return: returns a string containing all of the firmware information
    """
    url="https://{hostname}/xyz/openbmc_project/software/enumerate".format(hostname=host)
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    firmwareDict = json.loads(res.text)

    #sort the received information
    firmwareInfoDict = parseFWdata(firmwareDict)

    #display the information
    return displayFWInvenory(firmwareInfoDict, args)


def deleteFWVersion(host, args, session):
    """
         deletes a firmware version on the BMC

         @param host: string, the hostname or IP address of the BMC
         @param args: contains additional arguments used by the fwflash sub command
         @param session: the active session to use
         @param fwID: the unique ID of the fw version to delete
    """
    fwID = args.versionID

    print("Deleting version: "+fwID)
    url="https://"+host+"/xyz/openbmc_project/software/"+ fwID + "/action/Delete"
    data = "{\"data\": [] }"

    try:
        res = session.post(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    if res.status_code == 200:
        return ('The firmware version has been deleted')
    else:
        return ('Unable to delete the specified firmware version')


def restLogging(host, args, session):
    """
         Called by the logging function. Turns REST API logging on/off.

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the logging sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """
    url="https://"+host+"/xyz/openbmc_project/logging/rest_api_logs/attr/Enabled"

    if(args.rest_logging == 'on'):
        data = '{"data": 1}'
    elif(args.rest_logging == 'off'):
        data = '{"data": 0}'
    else:
        return "Invalid logging rest_api command"

    try:
        res = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    return res.text


def remoteLogging(host, args, session):
    """
         Called by the logging function. View config information for/disable remote logging (rsyslog).

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the logging sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """

    url="https://"+host+"/xyz/openbmc_project/logging/config/remote"

    try:
        if(args.remote_logging == 'view'):
            res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
        elif(args.remote_logging == 'disable'):
            res = session.put(url + '/attr/Port', headers=jsonHeader, json = {"data": 0}, verify=False, timeout=baseTimeout)
            res = session.put(url + '/attr/Address', headers=jsonHeader, json = {"data": ""}, verify=False, timeout=baseTimeout)
        else:
            return "Invalid logging remote_logging command"
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    return res.text


def remoteLoggingConfig(host, args, session):
    """
         Called by the logging function. Configures remote logging (rsyslog).

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the logging sub command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will be provided in json format for programmatic consumption
    """

    url="https://"+host+"/xyz/openbmc_project/logging/config/remote"

    try:
        res = session.put(url + '/attr/Port', headers=jsonHeader, json = {"data": args.port}, verify=False, timeout=baseTimeout)
        res = session.put(url + '/attr/Address', headers=jsonHeader, json = {"data": args.address}, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    return res.text

def redfishSupportPresent(host, session):
    url = "https://" + host + "/redfish/v1"
    try:
        resp = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return False
    except(requests.exceptions.ConnectionError) as err:
        return False
    if resp.status_code != 200:
        return False
    else:
       return True

def certificateUpdate(host, args, session):
    """
         Called by certificate management function. update server/client/authority certificates
         Example:
         certificate update server https -f cert.pem
         certificate update authority ldap -f Root-CA.pem
         certificate update client ldap -f cert.pem
         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the certificate update sub command
         @param session: the active session to use
    """
    httpHeader = {'Content-Type': 'application/octet-stream'}
    httpHeader.update(xAuthHeader)
    data = open(args.fileloc, 'r').read()
    try:
        if redfishSupportPresent(host, session):
            if(args.type.lower() == 'server' and args.service.lower() != "https"):
                return "Invalid service type"
            if(args.type.lower() == 'client' and args.service.lower() != "ldap"):
                return "Invalid service type"
            if(args.type.lower() == 'authority' and args.service.lower() != "ldap"):
                return "Invalid service type"
            url = "";
            if(args.type.lower() == 'server'):
                url = "https://" + host + \
                    "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates"
            elif(args.type.lower() == 'client'):
                url = "https://" + host + \
                    "/redfish/v1/AccountService/LDAP/Certificates"
            elif(args.type.lower() == 'authority'):
                url = "https://" + host + \
                "/redfish/v1/Managers/bmc/Truststore/Certificates"
            else:
                return "Unsupported certificate type"
            resp = session.post(url, headers=httpHeader, data=data,
                        verify=False)
        else:
            url = "https://" + host + "/xyz/openbmc_project/certs/" + \
                args.type.lower() + "/" + args.service.lower()
            resp = session.put(url, headers=httpHeader, data=data, verify=False)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if resp.status_code != 200:
        print(resp.text)
        return "Failed to update the certificate"
    else:
        print("Update complete.")

def certificateDelete(host, args, session):
    """
         Called by certificate management function to delete certificate
         Example:
         certificate delete server https
         certificate delete authority ldap
         certificate delete client ldap
         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the certificate delete sub command
         @param session: the active session to use
    """
    if redfishSupportPresent(host, session):
        return "Not supported, please use certificate replace instead";
    httpHeader = {'Content-Type': 'multipart/form-data'}
    httpHeader.update(xAuthHeader)
    url = "https://" + host + "/xyz/openbmc_project/certs/" + args.type.lower() + "/" + args.service.lower()
    print("Deleting certificate url=" + url)
    try:
        resp = session.delete(url, headers=httpHeader)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if resp.status_code != 200:
        print(resp.text)
        return "Failed to delete the certificate"
    else:
        print("Delete complete.")

def certificateReplace(host, args, session):
    """
         Called by certificate management function. replace server/client/
         authority certificates
         Example:
         certificate replace server https -f cert.pem
         certificate replace authority ldap -f Root-CA.pem
         certificate replace client ldap -f cert.pem
         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the certificate
                      replace sub command
         @param session: the active session to use
    """
    cert = open(args.fileloc, 'r').read()
    try:
        if redfishSupportPresent(host, session):
            httpHeader = {'Content-Type': 'application/json'}
            httpHeader.update(xAuthHeader)
            url = "";
            if(args.type.lower() == 'server' and args.service.lower() != "https"):
                return "Invalid service type"
            if(args.type.lower() == 'client' and args.service.lower() != "ldap"):
                return "Invalid service type"
            if(args.type.lower() == 'authority' and args.service.lower() != "ldap"):
                return "Invalid service type"
            if(args.type.lower() == 'server'):
                url = "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1"
            elif(args.type.lower() == 'client'):
                url = "/redfish/v1/AccountService/LDAP/Certificates/1"
            elif(args.type.lower() == 'authority'):
                url = "/redfish/v1/Managers/bmc/Truststore/Certificates/1"
            replaceUrl = "https://" + host + \
                "/redfish/v1/CertificateService/Actions/CertificateService.ReplaceCertificate"
            data ={"CertificateUri":{"@odata.id":url}, "CertificateType":"PEM",
                    "CertificateString":cert}
            resp = session.post(replaceUrl, headers=httpHeader, json=data, verify=False)
        else:
            httpHeader = {'Content-Type': 'application/octet-stream'}
            httpHeader.update(xAuthHeader)
            url = "https://" + host + "/xyz/openbmc_project/certs/" + \
                args.type.lower() + "/" + args.service.lower()
            resp = session.delete(url, headers=httpHeader)
            resp = session.put(url, headers=httpHeader, data=cert, verify=False)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if resp.status_code != 200:
        print(resp.text)
        return "Failed to replace the certificate"
    else:
        print("Replace complete.")
    return resp.text

def certificateDisplay(host, args, session):
    """
         Called by certificate management function. display server/client/
         authority certificates
         Example:
         certificate display server
         certificate display authority
         certificate display client
         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the certificate
                      display sub command
         @param session: the active session to use
    """
    if not redfishSupportPresent(host, session):
        return "Not supported";

    httpHeader = {'Content-Type': 'application/octet-stream'}
    httpHeader.update(xAuthHeader)
    if(args.type.lower() == 'server'):
        url = "https://" + host + \
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1"
    elif(args.type.lower() == 'client'):
        url = "https://" + host + \
            "/redfish/v1/AccountService/LDAP/Certificates/1"
    elif(args.type.lower() == 'authority'):
        url = "https://" + host + \
            "/redfish/v1/Managers/bmc/Truststore/Certificates/1"
    try:
        resp = session.get(url, headers=httpHeader, verify=False)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if resp.status_code != 200:
        print(resp.text)
        return "Failed to display the certificate"
    else:
        print("Display complete.")
    return resp.text

def certificateList(host, args, session):
    """
         Called by certificate management function.
         Example:
         certificate list
         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the certificate
                      list sub command
         @param session: the active session to use
    """
    if not redfishSupportPresent(host, session):
        return "Not supported";

    httpHeader = {'Content-Type': 'application/octet-stream'}
    httpHeader.update(xAuthHeader)
    url = "https://" + host + \
        "/redfish/v1/CertificateService/CertificateLocations/"
    try:
        resp = session.get(url, headers=httpHeader, verify=False)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if resp.status_code != 200:
        print(resp.text)
        return "Failed to list certificates"
    else:
        print("List certificates complete.")
    return resp.text

def certificateGenerateCSR(host, args, session):
    """
        Called by certificate management function. Generate CSR for server/
        client certificates
        Example:
        certificate generatecsr server NJ w3.ibm.com US IBM IBM-UNIT NY EC prime256v1 cp abc.com an.com,bm.com gn sn un in
        certificate generatecsr client NJ w3.ibm.com US IBM IBM-UNIT NY EC prime256v1 cp abc.com an.com,bm.com gn sn un in
        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the certificate replace sub command
        @param session: the active session to use
    """
    if not redfishSupportPresent(host, session):
        return "Not supported";

    httpHeader = {'Content-Type': 'application/octet-stream'}
    httpHeader.update(xAuthHeader)
    url = "";
    if(args.type.lower() == 'server'):
        url = "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/"
        usage_list = ["ServerAuthentication"]
    elif(args.type.lower() == 'client'):
        url = "/redfish/v1/AccountService/LDAP/Certificates/"
        usage_list = ["ClientAuthentication"]
    elif(args.type.lower() == 'authority'):
        url = "/redfish/v1/Managers/bmc/Truststore/Certificates/"
    print("Generating CSR url=" + url)
    generateCSRUrl = "https://" + host + \
        "/redfish/v1/CertificateService/Actions/CertificateService.GenerateCSR"
    try:
        alt_name_list = args.alternativeNames.split(",")
        data ={"CertificateCollection":{"@odata.id":url},
            "CommonName":args.commonName, "City":args.city,
            "Country":args.country, "Organization":args.organization,
            "OrganizationalUnit":args.organizationUnit, "State":args.state,
            "KeyPairAlgorithm":args.keyPairAlgorithm, "KeyCurveId":args.keyCurveId,
            "AlternativeNames":alt_name_list, "ContactPerson":args.contactPerson,
            "Email":args.email, "GivenName":args.givenname, "Initials":args.initials,
            "KeyUsage":usage_list, "Surname":args.surname,
            "UnstructuredName":args.unstructuredname}
        resp = session.post(generateCSRUrl, headers=httpHeader,
            json=data, verify=False)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if resp.status_code != 200:
        print(resp.text)
        return "Failed to generate CSR"
    else:
        print("GenerateCSR complete.")
    return resp.text

def enableLDAPConfig(host, args, session):
    """
         Called by the ldap function. Configures LDAP.

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will
            be provided in json format for programmatic consumption
    """

    if(isRedfishSupport):
        return enableLDAP(host, args, session)
    else:
        return enableLegacyLDAP(host, args, session)

def enableLegacyLDAP(host, args, session):
    """
         Called by the ldap function. Configures LDAP on Lagecy systems.

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will
            be provided in json format for programmatic consumption
    """

    url='https://'+host+'/xyz/openbmc_project/user/ldap/action/CreateConfig'
    scope = {
             'sub' : 'xyz.openbmc_project.User.Ldap.Create.SearchScope.sub',
             'one' : 'xyz.openbmc_project.User.Ldap.Create.SearchScope.one',
             'base': 'xyz.openbmc_project.User.Ldap.Create.SearchScope.base'
            }

    serverType = {
             'ActiveDirectory' : 'xyz.openbmc_project.User.Ldap.Create.Type.ActiveDirectory',
             'OpenLDAP' : 'xyz.openbmc_project.User.Ldap.Create.Type.OpenLdap'
            }

    data = {"data": [args.uri, args.bindDN, args.baseDN, args.bindPassword, scope[args.scope], serverType[args.serverType]]}

    try:
        res = session.post(url, headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

    return res.text

def enableLDAP(host, args, session):
    """
         Called by the ldap function. Configures LDAP for systems with latest user-manager design changes

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output will
            be provided in json format for programmatic consumption
    """

    scope = {
             'sub' : 'xyz.openbmc_project.User.Ldap.Config.SearchScope.sub',
             'one' : 'xyz.openbmc_project.User.Ldap.Config.SearchScope.one',
             'base': 'xyz.openbmc_project.User.Ldap.Config.SearchScope.base'
            }

    serverType = {
            'ActiveDirectory' : 'xyz.openbmc_project.User.Ldap.Config.Type.ActiveDirectory',
            'OpenLDAP' : 'xyz.openbmc_project.User.Ldap.Config.Type.OpenLdap'
            }

    url = "https://"+host+"/xyz/openbmc_project/user/ldap/"

    serverTypeEnabled = getLDAPTypeEnabled(host,session)
    serverTypeToBeEnabled = args.serverType

    #If the given LDAP type is already enabled, then return
    if (serverTypeToBeEnabled == serverTypeEnabled):
      return("Server type " + serverTypeToBeEnabled + " is already enabled...")

    try:

        #  Copy the role map from the currently enabled LDAP server type
        #  to the newly enabled server type
        #  Disable the currently enabled LDAP server type. Unless
        #  it is disabled, we cannot enable a new LDAP server type
        if (serverTypeEnabled is not None):

            if (serverTypeToBeEnabled != serverTypeEnabled):
                res = syncRoleMap(host,args,session,serverTypeEnabled,serverTypeToBeEnabled)

            data = "{\"data\": 0 }"
            res = session.put(url + serverTypeMap[serverTypeEnabled] + '/attr/Enabled', headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)

        data = {"data": args.baseDN}
        res = session.put(url + serverTypeMap[serverTypeToBeEnabled] + '/attr/LDAPBaseDN', headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
        if (res.status_code != requests.codes.ok):
            print("Updates to the property LDAPBaseDN failed...")
            return(res.text)

        data = {"data": args.bindDN}
        res = session.put(url + serverTypeMap[serverTypeToBeEnabled] + '/attr/LDAPBindDN', headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
        if (res.status_code != requests.codes.ok):
           print("Updates to the property LDAPBindDN failed...")
           return(res.text)

        data = {"data": args.bindPassword}
        res = session.put(url + serverTypeMap[serverTypeToBeEnabled] + '/attr/LDAPBindDNPassword', headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
        if (res.status_code != requests.codes.ok):
           print("Updates to the property LDAPBindDNPassword failed...")
           return(res.text)

        data = {"data": scope[args.scope]}
        res = session.put(url + serverTypeMap[serverTypeToBeEnabled] + '/attr/LDAPSearchScope', headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
        if (res.status_code != requests.codes.ok):
           print("Updates to the property LDAPSearchScope failed...")
           return(res.text)

        data = {"data": args.uri}
        res = session.put(url + serverTypeMap[serverTypeToBeEnabled] + '/attr/LDAPServerURI', headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
        if (res.status_code != requests.codes.ok):
           print("Updates to the property LDAPServerURI failed...")
           return(res.text)

        data = {"data": args.groupAttrName}
        res = session.put(url + serverTypeMap[serverTypeToBeEnabled] + '/attr/GroupNameAttribute', headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
        if (res.status_code != requests.codes.ok):
           print("Updates to the property GroupNameAttribute failed...")
           return(res.text)

        data = {"data": args.userAttrName}
        res = session.put(url + serverTypeMap[serverTypeToBeEnabled] + '/attr/UserNameAttribute', headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
        if (res.status_code != requests.codes.ok):
           print("Updates to the property UserNameAttribute failed...")
           return(res.text)

        #After updating the properties, enable the new server type
        data = "{\"data\": 1 }"
        res = session.put(url + serverTypeMap[serverTypeToBeEnabled] + '/attr/Enabled', headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)

    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    return res.text

def disableLDAP(host, args, session):
    """
         Called by the ldap function. Deletes the LDAP Configuration.

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output
            will be provided in json format for programmatic consumption
    """

    try:
        if (isRedfishSupport) :

            url = "https://"+host+"/xyz/openbmc_project/user/ldap/"

            serverTypeEnabled = getLDAPTypeEnabled(host,session)

            if (serverTypeEnabled is not None):
                #To keep the role map in sync,
                #If the server type being disabled has role map, then
                #   - copy the role map to the other server type(s)
                for serverType in serverTypeMap.keys():
                    if (serverType != serverTypeEnabled):
                        res = syncRoleMap(host,args,session,serverTypeEnabled,serverType)

                #Disable the currently enabled LDAP server type
                data = "{\"data\": 0 }"
                res = session.put(url + serverTypeMap[serverTypeEnabled] + '/attr/Enabled', headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)

            else:
                return("LDAP server has not been enabled...")

        else :
            url='https://'+host+'/xyz/openbmc_project/user/ldap/config/action/delete'
            data = {"data": []}
            res = session.post(url, headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)

    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

    return res.text

def enableDHCP(host, args, session):

    """
        Called by the network function. Enables DHCP.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/"+args.Interface+\
    "/attr/DHCPEnabled"
    data = "{\"data\": 1 }"
    try:
        res = session.put(url, headers=jsonHeader, data=data, verify=False,
                          timeout=baseTimeout)

    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 403:
        return "The specified Interface"+"("+args.Interface+")"+\
        " doesn't exist"

    return res.text


def disableDHCP(host, args, session):
    """
        Called by the network function. Disables DHCP.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/"+args.Interface+\
    "/attr/DHCPEnabled"
    data = "{\"data\": 0 }"
    try:
        res = session.put(url, headers=jsonHeader, data=data, verify=False,
                          timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 403:
        return "The specified Interface"+"("+args.Interface+")"+\
        " doesn't exist"
    return res.text


def getHostname(host, args, session):

    """
        Called by the network function. Prints out the Hostname.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/config/attr/HostName"

    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

    return res.text


def setHostname(host, args, session):
    """
        Called by the network function. Sets the Hostname.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/config/attr/HostName"

    data = {"data": args.HostName}

    try:
        res = session.put(url, headers=jsonHeader, json=data, verify=False,
                          timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

    return res.text


def getDomainName(host, args, session):

    """
        Called by the network function. Prints out the DomainName.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/"+args.Interface+\
    "/attr/DomainName"

    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "The DomainName is not configured on Interface"+"("+args.Interface+")"

    return res.text


def setDomainName(host, args, session):
    """
        Called by the network function. Sets the DomainName.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/"+args.Interface+\
    "/attr/DomainName"

    data = {"data": args.DomainName.split(",")}

    try:
        res = session.put(url, headers=jsonHeader, json=data, verify=False,
                          timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 403:
        return "The specified Interface"+"("+args.Interface+")"+\
        " doesn't exist"

    return res.text


def getMACAddress(host, args, session):

    """
        Called by the network function. Prints out the MACAddress.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/"+args.Interface+\
    "/attr/MACAddress"

    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "The specified Interface"+"("+args.Interface+")"+\
        " doesn't exist"

    return res.text


def setMACAddress(host, args, session):
    """
        Called by the network function. Sets the MACAddress.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/"+args.Interface+\
    "/attr/MACAddress"

    data = {"data": args.MACAddress}

    try:
        res = session.put(url, headers=jsonHeader, json=data, verify=False,
                          timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 403:
        return "The specified Interface"+"("+args.Interface+")"+\
        " doesn't exist"

    return res.text


def getDefaultGateway(host, args, session):

    """
        Called by the network function. Prints out the DefaultGateway.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/config/attr/DefaultGateway"

    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "Failed to get Default Gateway info!!"

    return res.text


def setDefaultGateway(host, args, session):
    """
        Called by the network function. Sets the DefaultGateway.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/config/attr/DefaultGateway"

    data = {"data": args.DefaultGW}

    try:
        res = session.put(url, headers=jsonHeader, json=data, verify=False,
                          timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 403:
        return "Failed to set Default Gateway!!"

    return res.text


def viewNWConfig(host, args, session):
    """
         Called by the ldap function. Prints out network configured properties

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
         @param session: the active session to use
         @return returns LDAP's configured properties.
    """
    url = "https://"+host+"/xyz/openbmc_project/network/enumerate"
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    except(requests.exceptions.RequestException) as err:
        return connectionErrHandler(args.json, "RequestException", err)
    if res.status_code == 404:
        return "LDAP server config has not been created"
    return res.text


def getDNS(host, args, session):

    """
        Called by the network function. Prints out DNS servers on the interface

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://" + host + "/xyz/openbmc_project/network/" + args.Interface\
        + "/attr/Nameservers"

    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "The NameServer is not configured on Interface"+"("+args.Interface+")"

    return res.text


def setDNS(host, args, session):
    """
        Called by the network function. Sets DNS servers on the interface.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://" + host + "/xyz/openbmc_project/network/" + args.Interface\
        + "/attr/Nameservers"

    data = {"data": args.DNSServers.split(",")}

    try:
        res = session.put(url, headers=jsonHeader, json=data, verify=False,
                          timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 403:
        return "The specified Interface"+"("+args.Interface+")" +\
            " doesn't exist"

    return res.text


def getNTP(host, args, session):

    """
        Called by the network function. Prints out NTP servers on the interface

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://" + host + "/xyz/openbmc_project/network/" + args.Interface\
        + "/attr/NTPServers"
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "The NTPServer is not configured on Interface"+"("+args.Interface+")"

    return res.text


def setNTP(host, args, session):
    """
        Called by the network function. Sets NTP servers on the interface.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://" + host + "/xyz/openbmc_project/network/" + args.Interface\
        + "/attr/NTPServers"

    data = {"data": args.NTPServers.split(",")}

    try:
        res = session.put(url, headers=jsonHeader, json=data, verify=False,
                          timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 403:
        return "The specified Interface"+"("+args.Interface+")" +\
            " doesn't exist"

    return res.text


def addIP(host, args, session):
    """
        Called by the network function. Configures IP address on given interface

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://" + host + "/xyz/openbmc_project/network/" + args.Interface\
        + "/action/IP"
    protocol = {
             'ipv4': 'xyz.openbmc_project.Network.IP.Protocol.IPv4',
             'ipv6': 'xyz.openbmc_project.Network.IP.Protocol.IPv6'
            }

    data = {"data": [protocol[args.type], args.address, int(args.prefixLength),
        args.gateway]}

    try:
        res = session.post(url, headers=jsonHeader, json=data, verify=False,
                           timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "The specified Interface" + "(" + args.Interface + ")" +\
            " doesn't exist"

    return res.text


def getIP(host, args, session):
    """
        Called by the network function. Prints out IP address of given interface

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://" + host+"/xyz/openbmc_project/network/" + args.Interface +\
        "/enumerate"
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "The specified Interface" + "(" + args.Interface + ")" +\
            " doesn't exist"

    return res.text


def deleteIP(host, args, session):
    """
        Called by the network function. Deletes the IP address from given Interface

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
        @param session: the active session to use
        @param args.json: boolean, if this flag is set to true, the output
            will be provided in json format for programmatic consumption
    """

    url = "https://"+host+"/xyz/openbmc_project/network/" + args.Interface+\
        "/enumerate"
    data = {"data": []}
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "The specified Interface" + "(" + args.Interface + ")" +\
            " doesn't exist"
    objDict = json.loads(res.text)
    if not objDict['data']:
        return "No object found for given address on given Interface"
    for obj in objDict['data']:
        try:
            if args.address in objDict['data'][obj]['Address']:
                url = "https://"+host+obj+"/action/Delete"
                try:
                    res = session.post(url, headers=jsonHeader, json=data,
                                       verify=False, timeout=baseTimeout)
                except(requests.exceptions.Timeout):
                    return(connectionErrHandler(args.json, "Timeout", None))
                except(requests.exceptions.ConnectionError) as err:
                    return connectionErrHandler(args.json, "ConnectionError", err)
                return res.text
            else:
                continue
        except KeyError:
            continue
    return "No object found for address " + args.address + \
           " on Interface(" + args.Interface + ")"


def addVLAN(host, args, session):
    """
        Called by the network function. Creates VLAN on given interface.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://" + host+"/xyz/openbmc_project/network/action/VLAN"

    data = {"data": [args.Interface,int(args.Identifier)]}
    try:
        res = session.post(url, headers=jsonHeader, json=data, verify=False,
                           timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 400:
        return "Adding VLAN to interface" + "(" + args.Interface + ")" +\
            " failed"

    return res.text


def deleteVLAN(host, args, session):
    """
        Called by the network function. Creates VLAN on given interface.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://" + host+"/xyz/openbmc_project/network/"+args.Interface+"/action/Delete"
    data = {"data": []}

    try:
        res = session.post(url, headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "The specified VLAN"+"("+args.Interface+")" +" doesn't exist"

    return res.text


def viewDHCPConfig(host, args, session):
    """
        Called by the network function. Shows DHCP configured Properties.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url="https://"+host+"/xyz/openbmc_project/network/config/dhcp"

    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

    return res.text


def configureDHCP(host, args, session):
    """
        Called by the network function. Configures/updates DHCP Properties.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """


    try:
        url="https://"+host+"/xyz/openbmc_project/network/config/dhcp"
        if(args.DNSEnabled == True):
            data = '{"data": 1}'
        else:
            data = '{"data": 0}'
        res = session.put(url + '/attr/DNSEnabled', headers=jsonHeader,
                          data=data, verify=False, timeout=baseTimeout)
        if(args.HostNameEnabled == True):
            data = '{"data": 1}'
        else:
            data = '{"data": 0}'
        res = session.put(url + '/attr/HostNameEnabled', headers=jsonHeader,
                          data=data, verify=False, timeout=baseTimeout)
        if(args.NTPEnabled == True):
            data = '{"data": 1}'
        else:
            data = '{"data": 0}'
        res = session.put(url + '/attr/NTPEnabled', headers=jsonHeader,
                          data=data, verify=False, timeout=baseTimeout)
        if(args.SendHostNameEnabled == True):
            data = '{"data": 1}'
        else:
            data = '{"data": 0}'
        res = session.put(url + '/attr/SendHostNameEnabled', headers=jsonHeader,
                          data=data, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

    return res.text


def nwReset(host, args, session):

    """
        Called by the network function. Resets networks setting to factory defaults.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
        @param session: the active session to use
    """

    url = "https://"+host+"/xyz/openbmc_project/network/action/Reset"
    data = '{"data":[] }'
    try:
        res = session.post(url, headers=jsonHeader, data=data, verify=False,
                          timeout=baseTimeout)

    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

    return res.text

def getLDAPTypeEnabled(host,session):

    """
        Called by LDAP related functions to find the LDAP server type that has been enabled.
        Returns None if LDAP has not been configured.

        @param host: string, the hostname or IP address of the bmc
        @param session: the active session to use
    """

    enabled = False
    url = 'https://'+host+'/xyz/openbmc_project/user/ldap/'
    for key,value in serverTypeMap.items():
        data = {"data": []}
        try:
            res = session.get(url + value + '/attr/Enabled', headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
        except(requests.exceptions.Timeout):
            print(connectionErrHandler(args.json, "Timeout", None))
            return
        except(requests.exceptions.ConnectionError) as err:
            print(connectionErrHandler(args.json, "ConnectionError", err))
            return

        enabled = res.json()['data']
        if (enabled):
            return key

def syncRoleMap(host,args,session,fromServerType,toServerType):

    """
        Called by LDAP related functions to sync the role maps
        Returns False if LDAP has not been configured.

        @param host: string, the hostname or IP address of the bmc
        @param session: the active session to use
        @param fromServerType : Server type whose role map has to be copied
        @param toServerType : Server type to which role map has to be copied
    """

    url = "https://"+host+"/xyz/openbmc_project/user/ldap/"

    try:
        #Note: If the fromServerType has no role map, then
        #the toServerType will not have any role map.

        #delete the privilege mapping from the toServerType and
        #then copy the privilege mapping from fromServerType to
        #toServerType.
        args.serverType = toServerType
        res = deleteAllPrivilegeMapping(host, args, session)

        data = {"data": []}
        res = session.get(url + serverTypeMap[fromServerType] + '/role_map/enumerate', headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
        #Previously enabled server type has no role map
        if (res.status_code != requests.codes.ok):

            #fromServerType has no role map; So, no need to copy
            #role map to toServerType.
            return

        objDict = json.loads(res.text)
        dataDict = objDict['data']
        for  key,value in dataDict.items():
            data = {"data": [value["GroupName"], value["Privilege"]]}
            res = session.post(url + serverTypeMap[toServerType] + '/action/Create', headers=jsonHeader, json = data, verify=False, timeout=baseTimeout)

    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    return res.text


def createPrivilegeMapping(host, args, session):
    """
         Called by the ldap function. Creates the group and the privilege mapping.

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
    """

    try:
        if (isRedfishSupport):
            url = 'https://'+host+'/xyz/openbmc_project/user/ldap/'

            #To maintain the interface compatibility between op930 and op940, the server type has been made
            #optional. If the server type is not specified, then create the role-mapper for the currently
            #enabled server type.
            serverType = args.serverType
            if (serverType is None):
                serverType = getLDAPTypeEnabled(host,session)
                if (serverType is None):
                     return("LDAP server has not been enabled. Please specify LDAP serverType to proceed further...")

            data = {"data": [args.groupName,args.privilege]}
            res = session.post(url + serverTypeMap[serverType] + '/action/Create', headers=jsonHeader, json = data, verify=False, timeout=baseTimeout)

        else:
            url = 'https://'+host+'/xyz/openbmc_project/user/ldap/action/Create'
            data = {"data": [args.groupName,args.privilege]}
            res = session.post(url, headers=jsonHeader, json = data, verify=False, timeout=baseTimeout)

    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    return res.text

def listPrivilegeMapping(host, args, session):
    """
         Called by the ldap function. Lists the group and the privilege mapping.

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
    """

    if (isRedfishSupport):
        serverType = args.serverType
        if (serverType is None):
            serverType = getLDAPTypeEnabled(host,session)
            if (serverType is None):
                return("LDAP has not been enabled. Please specify LDAP serverType to proceed further...")

        url = 'https://'+host+'/xyz/openbmc_project/user/ldap/'+serverTypeMap[serverType]+'/role_map/enumerate'

    else:
        url = 'https://'+host+'/xyz/openbmc_project/user/ldap/enumerate'

    data = {"data": []}

    try:
        res = session.get(url, headers=jsonHeader, json = data, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)

    return res.text

def deletePrivilegeMapping(host, args, session):
    """
         Called by the ldap function. Deletes the mapping associated with the group.

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
    """

    ldapNameSpaceObjects = listPrivilegeMapping(host, args, session)
    ldapNameSpaceObjects = json.loads(ldapNameSpaceObjects)["data"]
    path = ''
    data = {"data": []}

    if (isRedfishSupport):
        if (args.serverType is None):
            serverType = getLDAPTypeEnabled(host,session)
            if (serverType is None):
                return("LDAP has not been enabled. Please specify LDAP serverType to proceed further...")
        # search for the object having the mapping for the given group
        for key,value in ldapNameSpaceObjects.items():
            if value['GroupName'] == args.groupName:
                path = key
                break

        if path == '':
            return "No privilege mapping found for this group."

        # delete the object
        url = 'https://'+host+path+'/action/Delete'

    else:
        # not interested in the config objet
        ldapNameSpaceObjects.pop('/xyz/openbmc_project/user/ldap/config', None)

        # search for the object having the mapping for the given group
        for key,value in ldapNameSpaceObjects.items():
            if value['GroupName'] == args.groupName:
                path = key
                break

        if path == '':
            return "No privilege mapping found for this group."

        # delete the object
        url = 'https://'+host+path+'/action/delete'

    try:
        res = session.post(url, headers=jsonHeader, json = data, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    return res.text

def deleteAllPrivilegeMapping(host, args, session):
    """
         Called by the ldap function. Deletes all the privilege mapping and group defined.
         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
    """

    ldapNameSpaceObjects = listPrivilegeMapping(host, args, session)
    ldapNameSpaceObjects = json.loads(ldapNameSpaceObjects)["data"]
    path = ''
    data = {"data": []}

    if (isRedfishSupport):
        if (args.serverType is None):
            serverType = getLDAPTypeEnabled(host,session)
            if (serverType is None):
                return("LDAP has not been enabled. Please specify LDAP serverType to proceed further...")

    else:
        # Remove the config object.
        ldapNameSpaceObjects.pop('/xyz/openbmc_project/user/ldap/config', None)

    try:
        # search for GroupName property and delete if it is available.
        for path in ldapNameSpaceObjects.keys():
            # delete the object
            url = 'https://'+host+path+'/action/Delete'
            res = session.post(url, headers=jsonHeader, json = data, verify=False, timeout=baseTimeout)

    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    return res.text

def viewLDAPConfig(host, args, session):
    """
         Called by the ldap function. Prints out active LDAP configuration properties

         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the ldap subcommand
                args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
         @param session: the active session to use
         @return returns LDAP's configured properties.
    """

    try:
        if (isRedfishSupport):

            url = "https://"+host+"/xyz/openbmc_project/user/ldap/"

            serverTypeEnabled = getLDAPTypeEnabled(host,session)

            if (serverTypeEnabled is not None):
                data = {"data": []}
                res = session.get(url + serverTypeMap[serverTypeEnabled], headers=jsonHeader, json=data, verify=False, timeout=baseTimeout)
            else:
                return("LDAP server has not been enabled...")

        else :
            url = "https://"+host+"/xyz/openbmc_project/user/ldap/config"
            res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)

    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    if res.status_code == 404:
        return "LDAP server config has not been created"
    return res.text

def str2bool(v):
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

def localUsers(host, args, session):
    """
        Enables and disables local BMC users.

        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used by the logging sub command
        @param session: the active session to use
    """

    url="https://{hostname}/xyz/openbmc_project/user/enumerate".format(hostname=host)
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    usersDict = json.loads(res.text)

    if not usersDict['data']:
        return "No users found"

    output = ""
    for user in usersDict['data']:

        # Skip LDAP and another non-local users
        if 'UserEnabled' not in usersDict['data'][user]:
            continue

        name = user.split('/')[-1]
        url = "https://{hostname}{user}/attr/UserEnabled".format(hostname=host, user=user)

        if args.local_users == "queryenabled":
            try:
                res = session.get(url, headers=jsonHeader,verify=False, timeout=baseTimeout)
            except(requests.exceptions.Timeout):
                return(connectionErrHandler(args.json, "Timeout", None))

            result = json.loads(res.text)
            output += ("User: {name}  Enabled: {result}\n").format(name=name, result=result['data'])

        elif args.local_users in ["enableall", "disableall"]:
            action = ""
            if args.local_users == "enableall":
                data = '{"data": true}'
                action = "Enabling"
            else:
                data = '{"data": false}'
                action = "Disabling"

            output += "{action} {name}\n".format(action=action, name=name)

            try:
                resp = session.put(url, headers=jsonHeader, data=data, verify=False, timeout=baseTimeout)
            except(requests.exceptions.Timeout):
                return connectionErrHandler(args.json, "Timeout", None)
            except(requests.exceptions.ConnectionError) as err:
                return connectionErrHandler(args.json, "ConnectionError", err)
        else:
            return "Invalid local users argument"

    return output

def setPassword(host, args, session):
    """
         Set local user password
         @param host: string, the hostname or IP address of the bmc
         @param args: contains additional arguments used by the logging sub
                command
         @param session: the active session to use
         @param args.json: boolean, if this flag is set to true, the output
                will be provided in json format for programmatic consumption
         @return: Session object
    """
    try:
        if(isRedfishSupport):
            url = "https://" + host + "/redfish/v1/AccountService/Accounts/"+ \
                  args.user
            data = {"Password":args.password}
            res = session.patch(url, headers=jsonHeader, json=data,
                                verify=False, timeout=baseTimeout)
        else:
            url = "https://" + host + "/xyz/openbmc_project/user/" + args.user + \
                "/action/SetPassword"
            res = session.post(url, headers=jsonHeader,
                           json={"data": [args.password]}, verify=False,
                           timeout=baseTimeout)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    except(requests.exceptions.RequestException) as err:
        return connectionErrHandler(args.json, "RequestException", err)
    return res.status_code

def getThermalZones(host, args, session):
    """
        Get the available thermal control zones
        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used to get the thermal
               control zones
        @param session: the active session to use
        @return: Session object
    """
    url = "https://" + host + "/xyz/openbmc_project/control/thermal/enumerate"

    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=30)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    except(requests.exceptions.RequestException) as err:
        return connectionErrHandler(args.json, "RequestException", err)

    if (res.status_code == 404):
        return "No thermal control zones found"

    zonesDict = json.loads(res.text)
    if not zonesDict['data']:
        return "No thermal control zones found"
    for zone in zonesDict['data']:
        z = ",".join(str(zone.split('/')[-1]) for zone in zonesDict['data'])

    return "Zones: [ " + z + " ]"


def getThermalMode(host, args, session):
    """
        Get thermal control mode
        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used to get the thermal
               control mode
        @param session: the active session to use
        @param args.zone: the zone to get the mode on
        @return: Session object
    """
    url = "https://" + host + "/xyz/openbmc_project/control/thermal/" + \
        args.zone

    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=30)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    except(requests.exceptions.RequestException) as err:
        return connectionErrHandler(args.json, "RequestException", err)

    if (res.status_code == 404):
        return "Thermal control zone(" + args.zone + ") not found"

    propsDict = json.loads(res.text)
    if not propsDict['data']:
        return "No thermal control properties found on zone(" + args.zone + ")"
    curMode = "Current"
    supModes = "Supported"
    result = "\n"
    for prop in propsDict['data']:
        if (prop.casefold() == curMode.casefold()):
            result += curMode + " Mode: " + propsDict['data'][curMode] + "\n"
        if (prop.casefold() == supModes.casefold()):
            s = ", ".join(str(sup) for sup in propsDict['data'][supModes])
            result += supModes + " Modes: [ " + s + " ]\n"

    return result

def setThermalMode(host, args, session):
    """
        Set thermal control mode
        @param host: string, the hostname or IP address of the bmc
        @param args: contains additional arguments used for setting the thermal
               control mode
        @param session: the active session to use
        @param args.zone: the zone to set the mode on
        @param args.mode: the mode to enable
        @return: Session object
    """
    url = "https://" + host + "/xyz/openbmc_project/control/thermal/" + \
        args.zone + "/attr/Current"

    # Check args.mode against supported modes using `getThermalMode` output
    modes = getThermalMode(host, args, session)
    modes = os.linesep.join([m for m in modes.splitlines() if m])
    modes = modes.replace("\n", ";").strip()
    modesDict = dict(m.split(': ') for m in modes.split(';'))
    sModes = ''.join(s for s in modesDict['Supported Modes'] if s not in '[ ]')
    if args.mode.casefold() not in \
            (m.casefold() for m in sModes.split(',')) or not args.mode:
        result = ("Unsupported mode('" + args.mode + "') given, " +
                  "select a supported mode: \n" +
                  getThermalMode(host, args, session))
        return result

    data = '{"data":"' + args.mode + '"}'
    try:
        res = session.get(url, headers=jsonHeader, verify=False, timeout=30)
    except(requests.exceptions.Timeout):
        return(connectionErrHandler(args.json, "Timeout", None))
    except(requests.exceptions.ConnectionError) as err:
        return connectionErrHandler(args.json, "ConnectionError", err)
    except(requests.exceptions.RequestException) as err:
        return connectionErrHandler(args.json, "RequestException", err)

    if (data and res.status_code != 404):
        try:
            res = session.put(url, headers=jsonHeader,
                              data=data, verify=False,
                              timeout=30)
        except(requests.exceptions.Timeout):
            return(connectionErrHandler(args.json, "Timeout", None))
        except(requests.exceptions.ConnectionError) as err:
            return connectionErrHandler(args.json, "ConnectionError", err)
        except(requests.exceptions.RequestException) as err:
            return connectionErrHandler(args.json, "RequestException", err)

        if res.status_code == 403:
            return "The specified thermal control zone(" + args.zone + ")" + \
                " does not exist"

        return res.text
    else:
        return "Setting thermal control mode(" + args.mode + ")" + \
            " not supported or operation not available"


def createCommandParser():
    """
         creates the parser for the command line along with help for each command and subcommand

         @return: returns the parser for the command line
    """
    parser = argparse.ArgumentParser(description='Process arguments')
    parser.add_argument("-H", "--host", help='A hostname or IP for the BMC')
    parser.add_argument("-U", "--user", help='The username to login with')
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-A", "--askpw", action='store_true', help='prompt for password')
    group.add_argument("-P", "--PW", help='Provide the password in-line')
    group.add_argument("-E", "--PWenvvar", action='store_true', help='Get password from envvar OPENBMCTOOL_PASSWORD')
    parser.add_argument('-j', '--json', action='store_true', help='output json data only')
    parser.add_argument('-t', '--policyTableLoc', help='The location of the policy table to parse alerts')
    parser.add_argument('-c', '--CerFormat', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('-T', '--procTime', action='store_true', help= argparse.SUPPRESS)
    parser.add_argument('-V', '--version', action='store_true', help='Display the version number of the openbmctool')
    subparsers = parser.add_subparsers(title='subcommands', description='valid subcommands',help="sub-command help", dest='command')

    #fru command
    parser_inv = subparsers.add_parser("fru", help='Work with platform inventory')
    inv_subparser = parser_inv.add_subparsers(title='subcommands', description='valid inventory actions', help="valid inventory actions", dest='command')
    inv_subparser.required = True
    #fru print
    inv_print = inv_subparser.add_parser("print", help="prints out a list of all FRUs")
    inv_print.set_defaults(func=fruPrint)
    #fru list [0....n]
    inv_list = inv_subparser.add_parser("list", help="print out details on selected FRUs. Specifying no items will list the entire inventory")
    inv_list.add_argument('items', nargs='?', help="print out details on selected FRUs. Specifying no items will list the entire inventory")
    inv_list.set_defaults(func=fruList)
    #fru status
    inv_status = inv_subparser.add_parser("status", help="prints out the status of all FRUs")
    inv_status.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    inv_status.set_defaults(func=fruStatus)

    #sensors command
    parser_sens = subparsers.add_parser("sensors", help="Work with platform sensors")
    sens_subparser=parser_sens.add_subparsers(title='subcommands', description='valid sensor actions', help='valid sensor actions', dest='command')
    sens_subparser.required = True
    #sensor print
    sens_print= sens_subparser.add_parser('print', help="prints out a list of all Sensors.")
    sens_print.set_defaults(func=sensor)
    #sensor list[0...n]
    sens_list=sens_subparser.add_parser("list", help="Lists all Sensors in the platform. Specify a sensor for full details. ")
    sens_list.add_argument("sensNum", nargs='?', help="The Sensor number to get full details on" )
    sens_list.set_defaults(func=sensor)

    #thermal control commands
    parser_therm = subparsers.add_parser("thermal", help="Work with thermal control parameters")
    therm_subparser=parser_therm.add_subparsers(title='subcommands', description='Thermal control actions to work with', help='Valid thermal control actions to work with', dest='command')
    #thermal control zones
    parser_thermZones = therm_subparser.add_parser("zones", help="Get a list of available thermal control zones")
    parser_thermZones.set_defaults(func=getThermalZones)
    #thermal control modes
    parser_thermMode = therm_subparser.add_parser("modes", help="Work with thermal control modes")
    thermMode_sub = parser_thermMode.add_subparsers(title='subactions', description='Work with thermal control modes', help="Work with thermal control modes")
    #get thermal control mode
    parser_getThermMode = thermMode_sub.add_parser("get", help="Get current and supported thermal control modes")
    parser_getThermMode.add_argument('-z', '--zone', required=True, help='Thermal zone to work with')
    parser_getThermMode.set_defaults(func=getThermalMode)
    #set thermal control mode
    parser_setThermMode = thermMode_sub.add_parser("set", help="Set the thermal control mode")
    parser_setThermMode.add_argument('-z', '--zone', required=True, help='Thermal zone to work with')
    parser_setThermMode.add_argument('-m', '--mode', required=True, help='The supported thermal control mode')
    parser_setThermMode.set_defaults(func=setThermalMode)

    #sel command
    parser_sel = subparsers.add_parser("sel", help="Work with platform alerts")
    sel_subparser = parser_sel.add_subparsers(title='subcommands', description='valid SEL actions', help = 'valid SEL actions', dest='command')
    sel_subparser.required = True
    #sel print
    sel_print = sel_subparser.add_parser("print", help="prints out a list of all sels in a condensed list")
    sel_print.add_argument('-d', '--devdebug', action='store_true', help=argparse.SUPPRESS)
    sel_print.add_argument('-v', '--verbose', action='store_true', help="Changes the output to being very verbose")
    sel_print.add_argument('-f', '--fileloc', help='Parse a file instead of the BMC output')
    sel_print.set_defaults(func=selPrint)

    #sel list
    sel_list = sel_subparser.add_parser("list", help="Lists all SELs in the platform. Specifying a specific number will pull all the details for that individual SEL")
    sel_list.add_argument("selNum", nargs='?', type=int, help="The SEL entry to get details on")
    sel_list.set_defaults(func=selList)

    sel_get = sel_subparser.add_parser("get", help="Gets the verbose details of a specified SEL entry")
    sel_get.add_argument('selNum', type=int, help="the number of the SEL entry to get")
    sel_get.set_defaults(func=selList)

    sel_clear = sel_subparser.add_parser("clear", help="Clears all entries from the SEL")
    sel_clear.set_defaults(func=selClear)

    sel_setResolved = sel_subparser.add_parser("resolve", help="Sets the sel entry to resolved")
    sel_setResolved.add_argument('-n', '--selNum', type=int, help="the number of the SEL entry to resolve")
    sel_ResolveAll_sub = sel_setResolved.add_subparsers(title='subcommands', description='valid subcommands',help="sub-command help", dest='command')
    sel_ResolveAll = sel_ResolveAll_sub.add_parser('all', help='Resolve all SEL entries')
    sel_ResolveAll.set_defaults(func=selResolveAll)
    sel_setResolved.set_defaults(func=selSetResolved)

    parser_chassis = subparsers.add_parser("chassis", help="Work with chassis power and status")
    chas_sub = parser_chassis.add_subparsers(title='subcommands', description='valid subcommands',help="sub-command help", dest='command')

    parser_chassis.add_argument('status', action='store_true', help='Returns the current status of the platform')
    parser_chassis.set_defaults(func=chassis)

    parser_chasPower = chas_sub.add_parser("power", help="Turn the chassis on or off, check the power state")
    parser_chasPower.add_argument('powcmd',  choices=['on','softoff', 'hardoff', 'status'], help='The value for the power command. on, off, or status')
    parser_chasPower.set_defaults(func=chassisPower)

    #control the chassis identify led
    parser_chasIdent = chas_sub.add_parser("identify", help="Control the chassis identify led")
    parser_chasIdent.add_argument('identcmd', choices=['on', 'off', 'status'], help='The control option for the led: on, off, blink, status')
    parser_chasIdent.set_defaults(func=chassisIdent)

    #collect service data
    parser_servData = subparsers.add_parser("collect_service_data", help="Collect all bmc data needed for service")
    parser_servData.add_argument('-d', '--devdebug', action='store_true', help=argparse.SUPPRESS)
    parser_servData.set_defaults(func=collectServiceData)

    #system quick health check
    parser_healthChk = subparsers.add_parser("health_check", help="Work with platform sensors")
    parser_healthChk.set_defaults(func=healthCheck)

    #work with dumps
    parser_bmcdump = subparsers.add_parser("dump", help="Work with dumps")
    parser_bmcdump.add_argument("-t", "--dumpType", default='bmc', choices=['bmc','SystemDump'],help="Type of dump")
    bmcDump_sub = parser_bmcdump.add_subparsers(title='subcommands', description='valid subcommands',help="sub-command help", dest='command')
    bmcDump_sub.required = True
    dump_Create = bmcDump_sub.add_parser('create', help="Create a dump of given type")
    dump_Create.set_defaults(func=dumpCreate)

    dump_list = bmcDump_sub.add_parser('list', help="list all dumps")
    dump_list.set_defaults(func=dumpList)

    parserdumpdelete = bmcDump_sub.add_parser('delete', help="Delete dump")
    parserdumpdelete.add_argument("-n", "--dumpNum", nargs='*', type=int, help="The Dump entry to delete")
    parserdumpdelete.set_defaults(func=dumpDelete)

    bmcDumpDelsub = parserdumpdelete.add_subparsers(title='subcommands', description='valid subcommands',help="sub-command help", dest='command')
    deleteAllDumps = bmcDumpDelsub.add_parser('all', help='Delete all dumps')
    deleteAllDumps.set_defaults(func=dumpDeleteAll)

    parser_dumpretrieve = bmcDump_sub.add_parser('retrieve', help='Retrieve a dump file')
    parser_dumpretrieve.add_argument("-n,", "--dumpNum", help="The Dump entry to retrieve")
    parser_dumpretrieve.add_argument("-s", "--dumpSaveLoc", help="The location to save the bmc dump file or file path for system dump")
    parser_dumpretrieve.set_defaults(func=dumpRetrieve)

    #bmc command for reseting the bmc
    parser_bmc = subparsers.add_parser('bmc', help="Work with the bmc")
    bmc_sub = parser_bmc.add_subparsers(title='subcommands', description='valid subcommands',help="sub-command help", dest='command')
    parser_BMCReset = bmc_sub.add_parser('reset', help='Reset the bmc' )
    parser_BMCReset.add_argument('type', choices=['warm','cold'], help="Warm: Reboot the BMC, Cold: CLEAR config and reboot bmc")
    parser_bmc.add_argument('info', action='store_true', help="Displays information about the BMC hardware, including device revision, firmware revision, IPMI version supported, manufacturer ID, and information on additional device support.")
    parser_bmc.set_defaults(func=bmc)

    #add alias to the bmc command
    parser_mc = subparsers.add_parser('mc', help="Work with the management controller")
    mc_sub = parser_mc.add_subparsers(title='subcommands', description='valid subcommands',help="sub-command help", dest='command')
    parser_MCReset = mc_sub.add_parser('reset', help='Reset the bmc' )
    parser_MCReset.add_argument('type', choices=['warm','cold'], help="Reboot the BMC")
    #parser_MCReset.add_argument('cold', action='store_true', help="Reboot the BMC and CLEAR the configuration")
    parser_mc.add_argument('info', action='store_true', help="Displays information about the BMC hardware, including device revision, firmware revision, IPMI version supported, manufacturer ID, and information on additional device support.")
    parser_MCReset.set_defaults(func=bmcReset)
    parser_mc.set_defaults(func=bmc)

    #gard clear
    parser_gc = subparsers.add_parser("gardclear", help="Used to clear gard records")
    parser_gc.set_defaults(func=gardClear)

    #firmware_flash
    parser_fw = subparsers.add_parser("firmware", help="Work with the system firmware")
    fwflash_subproc = parser_fw.add_subparsers(title='subcommands', description='valid firmware commands', help='sub-command help', dest='command')
    fwflash_subproc.required = True

    fwflash = fwflash_subproc.add_parser('flash', help="Flash the system firmware")
    fwflash.add_argument('type', choices=['bmc', 'pnor'], help="image type to flash")
    fwflash.add_argument('-f', '--fileloc', required=True, help="The absolute path to the firmware image")
    fwflash.set_defaults(func=fwFlash)

    fwActivate = fwflash_subproc.add_parser('activate', help="Activate existing image on the bmc")
    fwActivate.add_argument('imageID', help="The image ID to activate from the firmware list. Ex: 63c95399")
    fwActivate.set_defaults(func=activateFWImage)

    fwActivateStatus = fwflash_subproc.add_parser('activation_status', help="Check Status of activations")
    fwActivateStatus.set_defaults(func=activateStatus)

    fwList = fwflash_subproc.add_parser('list', help="List all of the installed firmware")
    fwList.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    fwList.set_defaults(func=firmwareList)

    fwprint = fwflash_subproc.add_parser('print', help="List all of the installed firmware")
    fwprint.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    fwprint.set_defaults(func=firmwareList)

    fwDelete = fwflash_subproc.add_parser('delete', help="Delete an existing firmware version")
    fwDelete.add_argument('versionID', help="The version ID to delete from the firmware list. Ex: 63c95399")
    fwDelete.set_defaults(func=deleteFWVersion)

    #logging
    parser_logging = subparsers.add_parser("logging", help="logging controls")
    logging_sub = parser_logging.add_subparsers(title='subcommands', description='valid subcommands',help="sub-command help", dest='command')

    #turn rest api logging on/off
    parser_rest_logging = logging_sub.add_parser("rest_api", help="turn rest api logging on/off")
    parser_rest_logging.add_argument('rest_logging', choices=['on', 'off'], help='The control option for rest logging: on, off')
    parser_rest_logging.set_defaults(func=restLogging)

    #remote logging
    parser_remote_logging = logging_sub.add_parser("remote_logging", help="Remote logging (rsyslog) commands")
    parser_remote_logging.add_argument('remote_logging', choices=['view', 'disable'], help='Remote logging (rsyslog) commands')
    parser_remote_logging.set_defaults(func=remoteLogging)

    #configure remote logging
    parser_remote_logging_config = logging_sub.add_parser("remote_logging_config", help="Configure remote logging (rsyslog)")
    parser_remote_logging_config.add_argument("-a", "--address", required=True, help="Set IP address of rsyslog server")
    parser_remote_logging_config.add_argument("-p", "--port", required=True, type=int, help="Set Port of rsyslog server")
    parser_remote_logging_config.set_defaults(func=remoteLoggingConfig)

    #certificate management
    parser_cert = subparsers.add_parser("certificate", help="Certificate management")
    certMgmt_subproc = parser_cert.add_subparsers(title='subcommands', description='valid certificate commands', help='sub-command help', dest='command')

    certUpdate = certMgmt_subproc.add_parser('update', help="Update the certificate")
    certUpdate.add_argument('type', choices=['server', 'client', 'authority'], help="certificate type to update")
    certUpdate.add_argument('service', choices=['https', 'ldap'], help="Service to update")
    certUpdate.add_argument('-f', '--fileloc', required=True, help="The absolute path to the certificate file")
    certUpdate.set_defaults(func=certificateUpdate)

    certDelete = certMgmt_subproc.add_parser('delete', help="Delete the certificate")
    certDelete.add_argument('type', choices=['server', 'client', 'authority'], help="certificate type to delete")
    certDelete.add_argument('service', choices=['https', 'ldap'], help="Service to delete the certificate")
    certDelete.set_defaults(func=certificateDelete)

    certReplace = certMgmt_subproc.add_parser('replace',
        help="Replace the certificate")
    certReplace.add_argument('type', choices=['server', 'client', 'authority'],
        help="certificate type to replace")
    certReplace.add_argument('service', choices=['https', 'ldap'],
        help="Service to replace the certificate")
    certReplace.add_argument('-f', '--fileloc', required=True,
        help="The absolute path to the certificate file")
    certReplace.set_defaults(func=certificateReplace)

    certDisplay = certMgmt_subproc.add_parser('display',
        help="Print the certificate")
    certDisplay.add_argument('type', choices=['server', 'client', 'authority'],
        help="certificate type to display")
    certDisplay.set_defaults(func=certificateDisplay)

    certList = certMgmt_subproc.add_parser('list',
        help="Certificate list")
    certList.set_defaults(func=certificateList)

    certGenerateCSR = certMgmt_subproc.add_parser('generatecsr', help="Generate CSR")
    certGenerateCSR.add_argument('type', choices=['server', 'client', 'authority'],
        help="Generate CSR")
    certGenerateCSR.add_argument('city',
        help="The city or locality of the organization making the request")
    certGenerateCSR.add_argument('commonName',
        help="The fully qualified domain name of the component that is being secured.")
    certGenerateCSR.add_argument('country',
        help="The country of the organization making the request")
    certGenerateCSR.add_argument('organization',
        help="The name of the organization making the request.")
    certGenerateCSR.add_argument('organizationUnit',
        help="The name of the unit or division of the organization making the request.")
    certGenerateCSR.add_argument('state',
        help="The state, province, or region of the organization making the request.")
    certGenerateCSR.add_argument('keyPairAlgorithm',  choices=['RSA', 'EC'],
        help="The type of key pair for use with signing algorithms.")
    certGenerateCSR.add_argument('keyCurveId',
        help="The curve ID to be used with the key, if needed based on the value of the 'KeyPairAlgorithm' parameter.")
    certGenerateCSR.add_argument('contactPerson',
        help="The name of the user making the request")
    certGenerateCSR.add_argument('email',
        help="The email address of the contact within the organization")
    certGenerateCSR.add_argument('alternativeNames',
        help="Additional hostnames of the component that is being secured")
    certGenerateCSR.add_argument('givenname',
        help="The given name of the user making the request")
    certGenerateCSR.add_argument('surname',
        help="The surname of the user making the request")
    certGenerateCSR.add_argument('unstructuredname',
        help="he unstructured name of the subject")
    certGenerateCSR.add_argument('initials',
        help="The initials of the user making the request")
    certGenerateCSR.set_defaults(func=certificateGenerateCSR)

    # local users
    parser_users = subparsers.add_parser("local_users", help="Work with local users")
    parser_users.add_argument('local_users', choices=['disableall','enableall', 'queryenabled'], help="Disable, enable or query local user accounts")
    parser_users.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    parser_users.set_defaults(func=localUsers)

    #LDAP
    parser_ldap = subparsers.add_parser("ldap", help="LDAP controls")
    ldap_sub = parser_ldap.add_subparsers(title='subcommands', description='valid subcommands',help="sub-command help", dest='command')

    #configure and enable LDAP
    parser_ldap_config = ldap_sub.add_parser("enable", help="Configure and enables the LDAP")
    parser_ldap_config.add_argument("-a", "--uri", required=True, help="Set LDAP server URI")
    parser_ldap_config.add_argument("-B", "--bindDN", required=True, help="Set the bind DN of the LDAP server")
    parser_ldap_config.add_argument("-b", "--baseDN", required=True, help="Set the base DN of the LDAP server")
    parser_ldap_config.add_argument("-p", "--bindPassword", required=True, help="Set the bind password of the LDAP server")
    parser_ldap_config.add_argument("-S", "--scope", choices=['sub','one', 'base'],
            help='Specifies the search scope:subtree, one level or base object.')
    parser_ldap_config.add_argument("-t", "--serverType", required=True, choices=['ActiveDirectory','OpenLDAP'],
            help='Specifies the configured server is ActiveDirectory(AD) or OpenLdap')
    parser_ldap_config.add_argument("-g","--groupAttrName", required=False, default='', help="Group Attribute Name")
    parser_ldap_config.add_argument("-u","--userAttrName", required=False, default='', help="User Attribute Name")
    parser_ldap_config.set_defaults(func=enableLDAPConfig)

    # disable LDAP
    parser_disable_ldap = ldap_sub.add_parser("disable", help="disables the LDAP")
    parser_disable_ldap.set_defaults(func=disableLDAP)
    # view-config
    parser_ldap_config = \
    ldap_sub.add_parser("view-config", help="prints out a list of all \
                        LDAPS's configured properties")
    parser_ldap_config.set_defaults(func=viewLDAPConfig)

    #create group privilege mapping
    parser_ldap_mapper = ldap_sub.add_parser("privilege-mapper", help="LDAP group privilege controls")
    parser_ldap_mapper_sub = parser_ldap_mapper.add_subparsers(title='subcommands', description='valid subcommands',
            help="sub-command help", dest='command')

    parser_ldap_mapper_create = parser_ldap_mapper_sub.add_parser("create", help="Create mapping of ldap group and privilege")
    parser_ldap_mapper_create.add_argument("-t", "--serverType", choices=['ActiveDirectory','OpenLDAP'],
            help='Specifies the configured server is ActiveDirectory(AD) or OpenLdap')
    parser_ldap_mapper_create.add_argument("-g","--groupName",required=True,help="Group Name")
    parser_ldap_mapper_create.add_argument("-p","--privilege",choices=['priv-admin','priv-operator','priv-user','priv-callback'],required=True,help="Privilege")
    parser_ldap_mapper_create.set_defaults(func=createPrivilegeMapping)

    #list group privilege mapping
    parser_ldap_mapper_list = parser_ldap_mapper_sub.add_parser("list",help="List privilege mapping")
    parser_ldap_mapper_list.add_argument("-t", "--serverType", choices=['ActiveDirectory','OpenLDAP'],
            help='Specifies the configured server is ActiveDirectory(AD) or OpenLdap')
    parser_ldap_mapper_list.set_defaults(func=listPrivilegeMapping)

    #delete group privilege mapping
    parser_ldap_mapper_delete = parser_ldap_mapper_sub.add_parser("delete",help="Delete privilege mapping")
    parser_ldap_mapper_delete.add_argument("-t", "--serverType", choices=['ActiveDirectory','OpenLDAP'],
            help='Specifies the configured server is ActiveDirectory(AD) or OpenLdap')
    parser_ldap_mapper_delete.add_argument("-g","--groupName",required=True,help="Group Name")
    parser_ldap_mapper_delete.set_defaults(func=deletePrivilegeMapping)

    #deleteAll group privilege mapping
    parser_ldap_mapper_delete = parser_ldap_mapper_sub.add_parser("purge",help="Delete All privilege mapping")
    parser_ldap_mapper_delete.add_argument("-t", "--serverType", choices=['ActiveDirectory','OpenLDAP'],
            help='Specifies the configured server is ActiveDirectory(AD) or OpenLdap')
    parser_ldap_mapper_delete.set_defaults(func=deleteAllPrivilegeMapping)

    # set local user password
    parser_set_password = subparsers.add_parser("set_password",
        help="Set password of local user")
    parser_set_password.add_argument( "-p", "--password", required=True,
        help="Password of local user")
    parser_set_password.set_defaults(func=setPassword)

    # network
    parser_nw = subparsers.add_parser("network", help="network controls")
    nw_sub = parser_nw.add_subparsers(title='subcommands',
                                      description='valid subcommands',
                                      help="sub-command help",
                                      dest='command')

    # enable DHCP
    parser_enable_dhcp = nw_sub.add_parser("enableDHCP",
                                           help="enables the DHCP on given "
                                           "Interface")
    parser_enable_dhcp.add_argument("-I", "--Interface", required=True,
                                    help="Name of the ethernet interface(it can"
                                    "be obtained by the "
                                    "command:network view-config)"
                                    "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_enable_dhcp.set_defaults(func=enableDHCP)

    # disable DHCP
    parser_disable_dhcp = nw_sub.add_parser("disableDHCP",
                                            help="disables the DHCP on given "
                                            "Interface")
    parser_disable_dhcp.add_argument("-I", "--Interface", required=True,
                                     help="Name of the ethernet interface(it can"
                                     "be obtained by the "
                                     "command:network view-config)"
                                     "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_disable_dhcp.set_defaults(func=disableDHCP)

    # get HostName
    parser_gethostname = nw_sub.add_parser("getHostName",
                                           help="prints out HostName")
    parser_gethostname.set_defaults(func=getHostname)

    # set HostName
    parser_sethostname = nw_sub.add_parser("setHostName", help="sets HostName")
    parser_sethostname.add_argument("-H", "--HostName", required=True,
                                    help="A HostName for the BMC")
    parser_sethostname.set_defaults(func=setHostname)

    # get domainname
    parser_getdomainname = nw_sub.add_parser("getDomainName",
                                             help="prints out DomainName of "
                                             "given Interface")
    parser_getdomainname.add_argument("-I", "--Interface", required=True,
                                      help="Name of the ethernet interface(it "
                                      "can be obtained by the "
                                      "command:network view-config)"
                                      "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_getdomainname.set_defaults(func=getDomainName)

    # set domainname
    parser_setdomainname = nw_sub.add_parser("setDomainName",
                                             help="sets DomainName of given "
                                             "Interface")
    parser_setdomainname.add_argument("-D", "--DomainName", required=True,
                                      help="Ex: DomainName=Domain1,Domain2,...")
    parser_setdomainname.add_argument("-I", "--Interface", required=True,
                                      help="Name of the ethernet interface(it "
                                      "can be obtained by the "
                                      "command:network view-config)"
                                      "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_setdomainname.set_defaults(func=setDomainName)

    # get MACAddress
    parser_getmacaddress = nw_sub.add_parser("getMACAddress",
                                             help="prints out MACAddress the "
                                             "given Interface")
    parser_getmacaddress.add_argument("-I", "--Interface", required=True,
                                      help="Name of the ethernet interface(it "
                                      "can be obtained by the "
                                      "command:network view-config)"
                                      "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_getmacaddress.set_defaults(func=getMACAddress)

    # set MACAddress
    parser_setmacaddress = nw_sub.add_parser("setMACAddress",
                                             help="sets MACAddress")
    parser_setmacaddress.add_argument("-MA", "--MACAddress", required=True,
                                      help="A MACAddress for the given "
                                      "Interface")
    parser_setmacaddress.add_argument("-I", "--Interface", required=True,
                                    help="Name of the ethernet interface(it can"
                                    "be obtained by the "
                                    "command:network view-config)"
                                    "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_setmacaddress.set_defaults(func=setMACAddress)

    # get DefaultGW
    parser_getdefaultgw = nw_sub.add_parser("getDefaultGW",
                                            help="prints out DefaultGateway "
                                            "the BMC")
    parser_getdefaultgw.set_defaults(func=getDefaultGateway)

    # set DefaultGW
    parser_setdefaultgw = nw_sub.add_parser("setDefaultGW",
                                             help="sets DefaultGW")
    parser_setdefaultgw.add_argument("-GW", "--DefaultGW", required=True,
                                      help="A DefaultGateway for the BMC")
    parser_setdefaultgw.set_defaults(func=setDefaultGateway)

    # view network Config
    parser_ldap_config = nw_sub.add_parser("view-config", help="prints out a "
                                           "list of all network's configured "
                                           "properties")
    parser_ldap_config.set_defaults(func=viewNWConfig)

    # get DNS
    parser_getDNS = nw_sub.add_parser("getDNS",
                                      help="prints out DNS servers on the "
                                      "given interface")
    parser_getDNS.add_argument("-I", "--Interface", required=True,
                               help="Name of the ethernet interface(it can"
                               "be obtained by the "
                               "command:network view-config)"
                               "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_getDNS.set_defaults(func=getDNS)

    # set DNS
    parser_setDNS = nw_sub.add_parser("setDNS",
                                      help="sets DNS servers on the given "
                                      "interface")
    parser_setDNS.add_argument("-d", "--DNSServers", required=True,
                               help="Ex: DNSSERVERS=DNS1,DNS2,...")
    parser_setDNS.add_argument("-I", "--Interface", required=True,
                               help="Name of the ethernet interface(it can"
                               "be obtained by the "
                               "command:network view-config)"
                               "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_setDNS.set_defaults(func=setDNS)

    # get NTP
    parser_getNTP = nw_sub.add_parser("getNTP",
                                      help="prints out NTP servers on the "
                                      "given interface")
    parser_getNTP.add_argument("-I", "--Interface", required=True,
                               help="Name of the ethernet interface(it can"
                               "be obtained by the "
                               "command:network view-config)"
                               "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_getNTP.set_defaults(func=getNTP)

    # set NTP
    parser_setNTP = nw_sub.add_parser("setNTP",
                                      help="sets NTP servers on the given "
                                      "interface")
    parser_setNTP.add_argument("-N", "--NTPServers", required=True,
                               help="Ex: NTPSERVERS=NTP1,NTP2,...")
    parser_setNTP.add_argument("-I", "--Interface", required=True,
                               help="Name of the ethernet interface(it can"
                               "be obtained by the "
                               "command:network view-config)"
                               "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_setNTP.set_defaults(func=setNTP)

    # configure IP
    parser_ip_config = nw_sub.add_parser("addIP", help="Sets IP address to"
                                         "given interface")
    parser_ip_config.add_argument("-a", "--address", required=True,
                                  help="IP address of given interface")
    parser_ip_config.add_argument("-gw", "--gateway", required=False, default='',
                                  help="The gateway for given interface")
    parser_ip_config.add_argument("-l", "--prefixLength", required=True,
                                  help="The prefixLength of IP address")
    parser_ip_config.add_argument("-p", "--type", required=True,
                                  choices=['ipv4', 'ipv6'],
                                  help="The protocol type of the given"
                                  "IP address")
    parser_ip_config.add_argument("-I", "--Interface", required=True,
                                  help="Name of the ethernet interface(it can"
                                  "be obtained by the "
                                  "command:network view-config)"
                                  "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_ip_config.set_defaults(func=addIP)

    # getIP
    parser_getIP = nw_sub.add_parser("getIP", help="prints out IP address"
                                     "of given interface")
    parser_getIP.add_argument("-I", "--Interface", required=True,
                              help="Name of the ethernet interface(it can"
                              "be obtained by the command:network view-config)"
                              "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_getIP.set_defaults(func=getIP)

    # rmIP
    parser_rmIP = nw_sub.add_parser("rmIP", help="deletes IP address"
                                     "of given interface")
    parser_rmIP.add_argument("-a", "--address", required=True,
                                  help="IP address to remove form given Interface")
    parser_rmIP.add_argument("-I", "--Interface", required=True,
                             help="Name of the ethernet interface(it can"
                             "be obtained by the command:network view-config)"
                             "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_rmIP.set_defaults(func=deleteIP)

    # add VLAN
    parser_create_vlan = nw_sub.add_parser("addVLAN", help="enables VLAN "
                                           "on given interface with given "
                                           "VLAN Identifier")
    parser_create_vlan.add_argument("-I", "--Interface", required=True,
                                    choices=['eth0', 'eth1'],
                                    help="Name of the ethernet interface")
    parser_create_vlan.add_argument("-n", "--Identifier", required=True,
                                  help="VLAN Identifier")
    parser_create_vlan.set_defaults(func=addVLAN)

    # delete VLAN
    parser_delete_vlan = nw_sub.add_parser("deleteVLAN", help="disables VLAN "
                                           "on given interface with given "
                                           "VLAN Identifier")
    parser_delete_vlan.add_argument("-I", "--Interface", required=True,
                                    help="Name of the ethernet interface(it can"
                                    "be obtained by the "
                                    "command:network view-config)"
                                    "Ex: eth0 or eth1 or VLAN(VLAN=eth0_50 etc)")
    parser_delete_vlan.set_defaults(func=deleteVLAN)

    # viewDHCPConfig
    parser_viewDHCPConfig = nw_sub.add_parser("viewDHCPConfig",
                                              help="Shows DHCP configured "
                                              "Properties")
    parser_viewDHCPConfig.set_defaults(func=viewDHCPConfig)

    # configureDHCP
    parser_configDHCP = nw_sub.add_parser("configureDHCP",
                                          help="Configures/updates DHCP "
                                          "Properties")
    parser_configDHCP.add_argument("-d", "--DNSEnabled", type=str2bool,
                                   required=True, help="Sets DNSEnabled property")
    parser_configDHCP.add_argument("-n", "--HostNameEnabled", type=str2bool,
                                   required=True,
                                   help="Sets HostNameEnabled property")
    parser_configDHCP.add_argument("-t", "--NTPEnabled", type=str2bool,
                                   required=True,
                                   help="Sets NTPEnabled property")
    parser_configDHCP.add_argument("-s", "--SendHostNameEnabled", type=str2bool,
                                   required=True,
                                   help="Sets SendHostNameEnabled property")
    parser_configDHCP.set_defaults(func=configureDHCP)

    # network factory reset
    parser_nw_reset = nw_sub.add_parser("nwReset",
                                        help="Resets networks setting to "
                                        "factory defaults. "
                                        "note:Reset settings will be applied "
                                        "after BMC reboot")
    parser_nw_reset.set_defaults(func=nwReset)

    return parser

def main(argv=None):
    """
         main function for running the command line utility as a sub application
    """
    global toolVersion
    toolVersion = "1.19"
    global isRedfishSupport

    parser = createCommandParser()
    args = parser.parse_args(argv)

    totTimeStart = int(round(time.time()*1000))

    if(sys.version_info < (3,0)):
        urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
    if sys.version_info >= (3,0):
        requests.packages.urllib3.disable_warnings(requests.packages.urllib3.exceptions.InsecureRequestWarning)
    if (args.version):
        print("Version: "+ toolVersion)
        sys.exit(0)
    if (hasattr(args, 'fileloc') and args.fileloc is not None and 'print' in args.command):
        mysess = None
        print(selPrint('N/A', args, mysess))
    else:
        if(hasattr(args, 'host') and hasattr(args,'user')):
            if (args.askpw):
                pw = getpass.getpass()
            elif(args.PW is not None):
                pw = args.PW
            elif(args.PWenvvar):
                pw = os.environ['OPENBMCTOOL_PASSWORD']
            else:
                print("You must specify a password")
                sys.exit()
            logintimeStart = int(round(time.time()*1000))
            mysess = login(args.host, args.user, pw, args.json,
                           args.command == 'set_password')
            if(mysess == None):
                print("Login Failed!")
                sys.exit()
            if(sys.version_info < (3,0)):
                if isinstance(mysess, basestring):
                    print(mysess)
                    sys.exit(1)
            elif sys.version_info >= (3,0):
                if isinstance(mysess, str):
                    print(mysess)
                    sys.exit(1)
            logintimeStop = int(round(time.time()*1000))
            isRedfishSupport = redfishSupportPresent(args.host,mysess)
            commandTimeStart = int(round(time.time()*1000))
            output = args.func(args.host, args, mysess)
            commandTimeStop = int(round(time.time()*1000))
            if isinstance(output, dict):
                print(json.dumps(output, sort_keys=True, indent=4, separators=(',', ': '), ensure_ascii=False))
            else:
                print(output)
            if (mysess is not None):
                logout(args.host, args.user, pw, mysess, args.json)
            if(args.procTime):
                print("Total time: " + str(int(round(time.time()*1000))- totTimeStart))
                print("loginTime: " + str(logintimeStop - logintimeStart))
                print("command Time: " + str(commandTimeStop - commandTimeStart))
        else:
            print("usage:\n"
                  "  OPENBMCTOOL_PASSWORD=secret  # if using -E\n"
                  "  openbmctool.py [-h] -H HOST -U USER {-A | -P PW | -E} [-j]\n" +
                      "\t[-t POLICYTABLELOC] [-V]\n" +
                      "\t{fru,sensors,sel,chassis,collect_service_data, \
                          health_check,dump,bmc,mc,gardclear,firmware,logging}\n" +
                      "\t...\n" +
                      "openbmctool.py: error: the following arguments are required: -H/--host, -U/--user")
            sys.exit()

if __name__ == '__main__':
    """
         main function when called from the command line

    """
    import sys

    isTTY = sys.stdout.isatty()
    assert sys.version_info >= (2,7)
    main()
