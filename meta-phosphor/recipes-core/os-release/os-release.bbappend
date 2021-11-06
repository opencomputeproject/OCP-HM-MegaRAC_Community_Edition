# WARNING!
#
# These modifications to os-release disable the bitbake parse
# cache (for the os-release recipe only).  Before copying
# and pasting into another recipe ensure it is understood
# what that means!

def run_git(d, cmd):
    try:
        oeroot = d.getVar('COREBASE', True)
        return bb.process.run(("export PSEUDO_DISABLED=1; " +
                               "git --work-tree %s --git-dir %s/.git %s")
            % (oeroot, oeroot, cmd))[0].strip('\n')
    except Exception as e:
        bb.warn("Unexpected exception from 'git' call: %s" % e)
        pass

python() {
#    version_id = run_git(d, 'describe --dirty --long')
#    if version_id:
#        d.setVar('VERSION_ID', version_id)
#        versionList = version_id.split('-')
#        version = versionList[0] + "-" + versionList[1]
#        d.setVar('VERSION', version)

    stringToMajor = 'OBMC_FIRMWARE_ATTR_Major'
    stringToMinor = 'OBMC_FIRMWARE_ATTR_Minor'
    stringToAux = 'OBMC_FIRMWARE_ATTR_Aux'

    with open(d.getVar('PROJDEF_CFG'), 'r') as file:
        for line in file:
            if stringToMajor in line:
                matchedMajor = line.split('=')
                matchedMajor[1] = matchedMajor[1].replace('"', '')
                matchedMajor[1] = matchedMajor[1].replace('\n', '')

            elif stringToMinor in line:
               matchedMinor = line.split('=')
               matchedMinor[1] = matchedMinor[1].replace('"', '')
               matchedMinor[1] = matchedMinor[1].replace('\n', '')

            elif stringToAux in line:
               matchedAux = line.split('=')
               matchedAux[1] = matchedAux[1].replace('"', '')
               matchedAux[1] = matchedAux[1].replace('\n', '')


    version_id = matchedMajor[1] + "." + matchedMinor[1] + "." + matchedAux[1]
    version = matchedMajor[1] + "." + matchedMinor[1] + "." + matchedAux[1]

    if version_id:
        d.setVar('VERSION_ID', version_id)

    if version:
        d.setVar('VERSION', version)


    build_id = run_git(d, 'describe --abbrev=0')
    if build_id:
        d.setVar('BUILD_ID', build_id)

    target_machine = d.getVar('MACHINE', True)
    if target_machine:
        d.setVar('OPENBMC_TARGET_MACHINE', target_machine)
}

OS_RELEASE_FIELDS_append = " BUILD_ID OPENBMC_TARGET_MACHINE"

# Ensure the git commands run every time bitbake is invoked.
BB_DONT_CACHE = "1"

# Make os-release available to other recipes.
SYSROOT_DIRS_append = " ${sysconfdir}"
