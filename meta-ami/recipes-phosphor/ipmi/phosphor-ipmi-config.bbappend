FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append +=  "file://dev_id.json"
SRC_URI_append +=  "file://dcmi_cap.json"
SRC_URI_append +=  "file://power_reading.json"
SRC_URI_append +=  "file://channel_access.json"

unset do_patch[noexec]
python do_patch() {
    import json
    stringToMajor = 'OBMC_FIRMWARE_ATTR_Major'
    stringToMinor = 'OBMC_FIRMWARE_ATTR_Minor'
    stringToAux = 'OBMC_FIRMWARE_ATTR_Aux'
    with open(d.getVar('PROJDEF_CFG'), 'r') as file:
        for line in file:
            if stringToMajor in line:
                matchedMajor = line.split('=')
                matchedMajor[1] = matchedMajor[1].replace('"', '')
            elif stringToMinor in line:
                matchedMinor = line.split('=')
                matchedMinor[1] = matchedMinor[1].replace('"', '')
            elif stringToAux in line:
                matchedAux = line.split('=')
                matchedAux[1] = matchedAux[1].replace('"', '')
    workdir = d.getVar('WORKDIR', True)
    file = os.path.join(workdir, 'dev_id.json')
    # Update dev_id.json with the auxiliary firmware revision
    with open(file, "r+") as jsonFile:
        data = json.load(jsonFile)
        jsonFile.seek(0)
        jsonFile.truncate()
        data["fw0"] = int(matchedMajor[1])
        data["fw1"] = int(matchedMinor[1])
        data["aux"] = int(matchedAux[1])
        json.dump(data, jsonFile)
}
