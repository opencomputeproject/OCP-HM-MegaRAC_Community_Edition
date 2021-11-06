# Whitley CrashDump

## Overview

This source code implements crashdump for the Whitley platform and supports the
Ice Lake Server and Cooper Lake Server processors**. The crashdump application
collects debug data from the processors over the PECI bus when executed. Some
specific system level data may be collected as defined in the METADATA record
type. The intention of the usage model is that the crashDump application will
be called upon the BMC detecting an error event.

** Support is included in the code for Skylake and Cascades Lake processors
for reference and usage with an Whitley Platform SKX/CLX Interposer. This code
is not intended for production use on the Purley platform. This is due to
differences in the decoding tools used for the final JSON output.

## Implementation

### Source Description

This source consists of the main file, crashdump.cpp, a PECI interface library
of functions, a utility file, and a set of files under the directory
CrashDumpSections that each are used for the specific record types of the
output file. The record types generated are as follows:

- METADATA
- address_map
- big_core
- MCA
- PM_Info
- TOR
- Uncore

### System Requirements

### Initialization/Start-Up

It is expected that the crashdump application is ready/running and avaliable to
be called after BMC has been booted.

### System Reset Avoidence

System resets and/or power cycling the CPU's in reaction to an error event must
be disabled or delayed until after crashdump collection has completed.

### PECI Interface Library (libpeci.c)

These functions serve as an interface to the PECI Driver and provide a higher
level interface to the PECI processor commands used for the extraction of data.

### Standalone PECI Executable (peci_cmds.c)

A standalone PECI interface is provided for manual testing of commands, see
the Usage comments in the file for input parameter definitions.

### Data Extraction Dependencies

PECI Access to record data is restricted according to the following table.

| Record      | Gate    |
| ---         | ---     |
| METADATA    | None    |
| Address_map | None    |
| Big_Core    | 3strike |
| MCA         | None    |
| PM_Info     | None    |
| TOR         | IERR    |
| Uncore      | None    |
|             |         |

### File Output Format

The output is stored in the JSON output format.

Example: (*shows a subset of the data collected in the METADATA record*)

```json
{
    "crash_data": {
        "METADATA": {
            "crashdump_ver": "BMC_1924",
        "cpu0": {
            "cores_per_cpu": "0xc",
            "cpuid": "0x606a0",
            "peci_id": "0x30",
        },
        "cpu1": {
            "cores_per_cpu": "0xc",
            "cpuid": "0x606a0",
            "peci_id": "0x31",
        }
    }
}
```

## Executing CrashDump

There are two ways to execute CrashDump.

- On-Demand
  - /redfish/v1/Systems/system/LogServices/Crashdump/
    Actions/Oem/Crashdump.OnDemand
- Direct call upon detection of an Error Event, Possible Error events
  - IERR
  - ERR2
  - SMI Timeout

## Extracting Output

### On-Demand CrashDump

For Intel OpenBMC, a redfish command can be used for a requesting and extracting
an immediate snapshot of the CrashDump output. This code both requests the data
to be acquired and for the file to be extracted. This request is handled through
the following command.

- /redfish/v1/Systems/system/LogServices/Crashdump/Actions/Oem/
  Crashdump.OnDemand

## Extracting the Output file(s)

For Intel OpenBMC, The method to extract the autonomously triggered crashdump is
to use the following redfish command:

- /redfish/v1/Systems/system/LogServices/Crashdump/Entries

The result of this command is an array of the available Crashdump entries with
their links. When you request each link, you will get the Crashdump file.

## Schema file

TBD

## Interpreting Output

The Whitley decoder/analyzer/summarizer tools is provide in Whitley CScripts.
See CScripts python Script:

- \cscripts\icelakex\toolext\cd_summarizer.py

## Security/Privacy

The big_core record type contains architectural register information from the
core and the information in these register could be private.

Examples of big_core data collected.

- Last Branch Records
- Last Execption Records
- General Registers: RAX,RBX,RCX..., R8, R9, R10...
- Instruction Pointer: LIP
- Selectors: CS,DS,ES,FS,GS,SS
- MTRR Registers
- Control registers: CR0,CR2,CR3,CR4
- Other Misc. control registers

## References

- Icelake Server External Design Specification (EDS), Volume One: Architecture
    (Document Number: 574451)
- Icelake Server External Design Specification (EDS), Volume Two: Registers
    (Document Number: 574942)
- Cooper Lake Processor External Design Specification (EDS), Volume One
    Architecture (Document Number: 604785)
- Cooper Lake Processor External Design Specification (EDS), Volume Two
    Registers Part A (Document Number: 604926)
- Cooper Lake Processor External Design Specification (EDS), Volume Two
    Registers Part B (Document Number: 605073)
