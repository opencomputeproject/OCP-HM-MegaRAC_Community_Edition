def get_devnet_feature(property, ifenabled, ifdisabled, d):
    with open(d.getVar('PROJDEF_CFG'), 'r') as file:
        for line in file:
            if property in line:
                val = line.split("=")[1].strip().strip("\"")
                if val == "YES":
                    return ifenabled
                else:
                    return ifdisabled
        return ifdisabled

