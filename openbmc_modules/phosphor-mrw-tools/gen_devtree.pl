#!/usr/bin/env perl

#Generates an OpenBMC device tree syntax file from the machine
#readable workbook.  It relies on the fact that the dts include
#file for the BMC chip itself contains the BMC chip specific
#data, so this can just generate the system specific data.
#It also makes use of a YAML configuration file to contain
#settings that are outside the scope of the MRW.
#
#This doesn't attempt to support every possible type of device
#tree node from the start.  Support may need to be added as newer
#systems come along that make use of different features.

use strict;
use warnings;
use XML::Simple;
use mrw::Targets;
use mrw::Util;
use Getopt::Long;
use YAML::Tiny qw(LoadFile);
use Scalar::Util qw(looks_like_number);

use constant {
    VERSION => "/dts-v1/;",
    ZERO_LENGTH_PROPERTY => "zero_length_property",
    PRE_ROOT_INCLUDES => "pre-root-node",
    ROOT_INCLUDES => "root-node",
    POST_ROOT_INCLUDES => "post-root-node",
    HOST_SPI_FLASH_MEM_REGION_NODE_LABEL => "flash_memory"
};


my $serverwizFile;
my $configFile;
my $outputFile;
my $debug;

GetOptions("x=s" => \$serverwizFile,
           "y=s" => \$configFile,
           "o=s" => \$outputFile,
           "d" => \$debug)
or printUsage();

if ((not defined $serverwizFile) || (not defined $outputFile) ||
    (not defined $configFile)) {
    printUsage();
}

my $g_pnorNodeName = undef;
my %g_configuration = %{ LoadFile($configFile) };

my $g_targetObj = Targets->new;
$g_targetObj->loadXML($serverwizFile);

my ($g_bmc, $g_bmcModel, $g_bmcMfgr, $g_systemName);
setGlobalAttributes();

my $g_i2cBusAdjust = 0;
getI2CBusAdjust();

open (my $f, ">$outputFile") or die "Could not open $outputFile\n";

printVersion($f);
printIncludes($f, PRE_ROOT_INCLUDES);
printRootNodeStart($f);

printPropertyList($f, 1, "model", getSystemBMCModel());
printPropertyList($f, 1, "compatible", getBMCCompatibles());

printNode($f, 1, "aliases", getAliases());
printNode($f, 1, "chosen", getChosen());
printNode($f, 1, "memory", getBmcMemory());
printNode($f, 1, "reserved-memory", getReservedMemory());

printNode($f, 1, "leds", getLEDNode());

printNode($f, 1, "fsi-master", getFSINode());

printIncludes($f, ROOT_INCLUDES);

printRootNodeEnd($f, 0);

printNodes($f, 0, getBMCFlashNodes());
printNodes($f, 0, getOtherFlashNodes());

printNode($f, 0, "lpc_ctrl", getLPCNode());
printNode($f, 0, "mbox", getMBoxNode());

printNodes($f, 0, getUARTNodes());
printNodes($f, 0, getMacNodes());

printNodes($f, 0, getI2CNodes());
printNodes($f, 0, getVuartNodes());

printIncludes($f, POST_ROOT_INCLUDES);

close $f;
exit 0;


#Finds the values for these globals:
# $g_bmc, $g_bmcModel, $g_bmcMfgr, $g_systemName
sub setGlobalAttributes
{
    $g_bmc = Util::getBMCTarget($g_targetObj);

    if ($g_targetObj->isBadAttribute($g_bmc, "MODEL")) {
        die "The MODEL attribute on $g_bmc is missing or empty.\n";
    }
    $g_bmcModel = $g_targetObj->getAttribute($g_bmc, "MODEL");

    if ($g_targetObj->isBadAttribute($g_bmc, "MANUFACTURER")) {
        die "The MANUFACTURER attribute on $g_bmc is missing or empty.\n";
    }
    $g_bmcMfgr = $g_targetObj->getAttribute($g_bmc, "MANUFACTURER");

    $g_systemName = $g_targetObj->getSystemName();
    if (length($g_systemName) == 0) {
        die "The SYSTEM_NAME attribute is not set on the system target.\n";
    }
}


#Returns a hash that represents the 'aliases' node.
#Will look like:
#  aliases {
#    name1 = &val1;
#    name2 = &val2;
#    ...
#  }
sub getAliases
{
    my %aliases;

    #Get the info from the config file

    if ((not exists $g_configuration{aliases}) ||
        (keys %{$g_configuration{aliases}} == 0)) {
        print "WARNING:  Missing or empty 'aliases' section in config file.\n";
        return %aliases;
    }
    %aliases = %{ $g_configuration{aliases} };

    #add a & reference if one is missing
    foreach my $a (keys %aliases) {
        if (($aliases{$a} !~ /^&/) && ($aliases{$a} !~ /^\(ref\)/)) {
            $aliases{$a} = "(ref)$aliases{$a}";
        }
    }

    return %aliases;
}


#Return a hash that represents the 'chosen' node
#Will look like:
#   chosen {
#      stdout-path = ...
#      bootargs = ...
#   }
sub getChosen
{
    my %chosen;
    my @allowed = qw(bootargs stdin-path stdout-path);

    #Get the info from the config file

    if (not exists $g_configuration{chosen}) {
        die "ERROR:  Missing 'chosen' section in config file.\n";
    }
    %chosen = %{ $g_configuration{chosen} };

    foreach my $key (keys %chosen) {

        #Check for allowed entries.  Empty is OK.
        if (!grep(/^$key$/, @allowed)) {
            die "Invalid entry $key in 'chosen' section in config file\n";
        }

        #stdout-path and stdin-path can use aliases, which will look like
        #(alias)uart5 in the yaml.  Change to (ref)uart5 so it will be
        #converted to a '&' later.
        $chosen{$key} =~ s/\(alias\)/\(ref\)/g;
    }

    return %chosen;
}


#Return a hash that represents the 'memory' node.
#Will look like:
#  memory {
#     reg = < base size >
#  }
sub getBmcMemory
{
    my %memory;

    #Get the info from the config file

    if (not exists $g_configuration{memory}) {
        die "ERROR:  Missing 'memory' section in config file.\n";
    }

    if ((not exists $g_configuration{memory}{base}) ||
        ($g_configuration{memory}{base} !~ /0x/)) {
        die "ERROR:  The base entry in the memory section in the config " .
            "file is either missing or invalid.\n";
    }

    if ((not exists $g_configuration{memory}{size}) ||
        ($g_configuration{memory}{size} !~ /0x/)) {
        die "ERROR:  The size entry in the memory section in the config " .
            "file is either missing or invalid.\n";
    }

    #Future: could do more validation on the actual values

    addRegProp(\%memory,
               $g_configuration{memory}{base},
               $g_configuration{memory}{size});

    return %memory;
}


#Returns a hash that represents the 'reserved-memory' node.
#This currently only supports the memory region for the LPC
#host spi flash mailbox.  Will look like:
#  reserved-memory {
#     #address-cells = <1>;
#     #size-cells = <1>;
#     ranges;
#
#     flash_memory: region@94000000 {
#       no-map;
#       reg = <0x94000000 0x04000000>;
#     };
#  };
sub getReservedMemory
{
    my %memory;

    if (not exists $g_configuration{"lpc-host-spi-flash-mailbox"}) {
        return %memory;
    }

    $memory{"#address-cells"} = "<1>";
    $memory{"#size-cells"} = "<1>";
    $memory{ranges} = ZERO_LENGTH_PROPERTY;

    #Get the sub node that contains the address range
    my ($name, $node) = getHostSpiFlashMboxRegion();
    $memory{$name} = { %$node };

    return %memory;
}


#Returns a hash that represents a child node of the
#reserved-memory node which contains the address range
#that the host spi flash is mapped to.
sub getHostSpiFlashMboxRegion
{
    my %node;

    $node{"no-map"} = ZERO_LENGTH_PROPERTY;

    #This node needs a label the LPC node can refer to.
    $node{NODE_LABEL} = HOST_SPI_FLASH_MEM_REGION_NODE_LABEL;

    #Get the memory region's base address and size from the config file
    if (not exists $g_configuration{"lpc-host-spi-flash-mailbox"}
                                   {"bmc-address-range"}{base}) {
        die "Could not find lpc-host-spi-flash-mailbox base " .
            "address in config file\n";
    }

    my $base = $g_configuration{"lpc-host-spi-flash-mailbox"}
                               {"bmc-address-range"}{base};
    #Allow 1 hex value, up to 4B
    if ($base !~ /^0x[0-9a-fA-F]{1,8}$/) {
        die "lpc-host-spi-flash-mailbox base address $base is invalid\n";
    }

    if (not exists $g_configuration{"lpc-host-spi-flash-mailbox"}
                                   {"bmc-address-range"}{size}) {
        die "Could not find lpc-host-spi-flash-mailbox address size " .
            "in config file\n";
    }

    my $size = $g_configuration{"lpc-host-spi-flash-mailbox"}
                               {"bmc-address-range"}{size};
    if ($size !~ /^0x[0-9a-fA-F]{1,8}$/) {
        die "lpc-host-spi-flash-mailbox address range size " .
            "$size is invalid\n";
    }

    addRegProp(\%node, $base, $size);
    my $name = makeNodeName("region", $node{reg});

    return ($name, \%node);
}


#Returns an array of hashes representing the device tree nodes for
#the BMC flash.  These nodes are BMC model specific because different
#models can have different device drivers.
sub getBMCFlashNodes
{
    my @nodes;

    if ($g_bmcModel eq "AST2500") {
        my %node = getAST2500BMCSPIFlashNode();
        push @nodes, { %node };
    }
    else {
        die "ERROR:  No BMC SPI flash support yet for BMC model $g_bmcModel\n";
    }

    return @nodes;
}


#Returns a hash that represents the BMC SPI flash(es) by finding the SPI
#connections that come from the unit tagged as BMC_CODE.  The code also
#looks in the config file for any additional properties to add.  Supports
#the hardware where the same SPI master unit can be wired to more than 1
#flash (a chip select line is used to switch between them.)  This is
#specific to the ASPEED AST2500 hardware and device driver.
#Will look like:
#  fmc {
#    status = "okay"
#    flash@0 {
#       ...
#    };
#    flash@1 {
#       ...
#    };
sub getAST2500BMCSPIFlashNode
{
    my %bmcFlash;
    my $chipSelect = 0;
    my $lastUnit = "";

    my $connections = $g_targetObj->findConnections($g_bmc, "SPI", "FLASH");

    if ($connections eq "") {
        die "ERROR:  No BMC SPI flashes found connected to the BMC\n";
    }

    statusOK(\%{$bmcFlash{fmc}});

    foreach my $spi (@{$connections->{CONN}}) {

        #Looking for spi-masters with a function of 'BMC_CODE'.
        #It's possible there are multiple flash chips here.
        if (!$g_targetObj->isBadAttribute($spi->{SOURCE}, "SPI_FUNCTION")) {

            my $function = $g_targetObj->getAttribute($spi->{SOURCE},
                                                      "SPI_FUNCTION");
            if ($function eq "BMC_CODE") {

                my $flashName = "flash@".$chipSelect;

                $bmcFlash{fmc}{$flashName}{COMMENT} = connectionComment($spi);

                statusOK(\%{$bmcFlash{fmc}{$flashName}});

                #Add in anything specified in the config file for this chip.
                addBMCFlashConfigProperties(\%{$bmcFlash{fmc}{$flashName}},
                                            $chipSelect);

                #The code currently only supports the config where a chip
                #select line is used to select between possibly multiple
                #flash chips attached to the same SPI pins/unit.  So we
                #need to make sure if there are multiple chips found, that
                #they are off of the same master unit.
                if ($lastUnit eq "") {
                    $lastUnit = $spi->{SOURCE};
                }
                else {
                    if ($lastUnit ne $spi->{SOURCE}) {
                        die "ERROR:  Currently only 1 spi-master unit is " .
                            "supported for BMC flash connections."
                    }
                }

                #Since we don't need anything chip select specific from the
                #XML, we can just assign our own chip selects.
                $chipSelect++;
            }
        }
    }

    if ($chipSelect == 0) {
        die "ERROR:  Didn't find any BMC flash chips connected";
    }

    return %bmcFlash;
}


#Looks in the bmc-flash-config section in the config file for the
#chip select passed in to add any additional properties to the BMC
#flash node.
#  $node = hash reference to the flash node
#  $cs = the flash chip select value
sub addBMCFlashConfigProperties
{
    my ($node, $cs) = @_;
    my $section = "chip-select-$cs";

    if (exists $g_configuration{"bmc-flash-config"}{$section}) {
        foreach my $key (sort keys $g_configuration{"bmc-flash-config"}{$section}) {
            $node->{$key} = $g_configuration{"bmc-flash-config"}{$section}{$key};
        }
    }
}


#Returns an array of hashes representing the other flashes used by the
#BMC besides the ones that hold the BMC code.  This is BMC model specific
#as different models can have different interfaces.
#Typically, these are SPI flashes.
sub getOtherFlashNodes
{
    my @nodes;

    if ($g_bmcModel eq "AST2500") {
        @nodes = getAST2500SpiFlashNodes();
    }
    else {
        die "ERROR:  No SPI flash support yet for BMC model $g_bmcModel\n";
    }

    return @nodes;
}


#Returns an array of hashes representing the SPI flashes in an
#AST2500.  These are for the SPI1 and SPI2 interfaces in the chip.
#Each SPI master interface can support multiple flash chips.  If
#no hardware is connected to the interface, the node won't be present.
sub getAST2500SpiFlashNodes
{
    my @nodes;

    #The AST2500 has 2 SPI master units, 1 and 2.
    my @units = (1, 2);

    foreach my $unit (@units) {

        my ($node, $foundPNOR) = getAST2500SpiMasterNode($unit);

        if (keys %$node) {
            my %spiNode;
            my $nodeName = "spi$unit";
            $spiNode{$nodeName} = { %$node };
            push @nodes, { %spiNode };

            #Save off the PNOR SPI node name for use by LPC node
            if ($foundPNOR) {
                $g_pnorNodeName = $nodeName;
            }
        }
    }

    return @nodes;
}


#Returns a hash that represents the device tree node for the SPI1
#or SPI2 master interface on the AST2500.  Each master can support
#multiple chips by use of a chip select.
#Will look like:
#  spi1 {
#    status = "okay";
#    flash@0 {
#       ...
#    };
#  };
#
#  $spiNum = The SPI master unit number to use
sub getAST2500SpiMasterNode
{
    my $spiNum = shift;
    my %spiMaster;
    my $chipSelect = 0;
    my $foundPNOR = 0;

    my $connections = $g_targetObj->findConnections($g_bmc, "SPI", "FLASH");

    if ($connections eq "") {
        return %spiMaster;
    }

    #Looking for spi-masters with a chip-unit of $spiNum
    #It's possible there are multiple flash chips off the master
    foreach my $spi (@{$connections->{CONN}}) {

        my $unitNum = $g_targetObj->getAttribute($spi->{SOURCE},
                                                 "CHIP_UNIT");
        if ($unitNum == $spiNum) {
            statusOK(\%spiMaster);

            #Add in any pinctrl properties.  These would come from the parent
            #of $spi{SOURCE}, which would be a unit-pingroup-bmc if the
            #pins for this connection are multi-function.
            addPinCtrlProps($g_targetObj->getTargetParent($spi->{SOURCE}),
                            \%spiMaster);

            my $flashName = "flash@".$chipSelect;

            $spiMaster{$flashName}{COMMENT} = connectionComment($spi);

            statusOK(\%{$spiMaster{$flashName}});

            #AST2500 PNORs need a label
            my $function = $g_targetObj->getAttribute($spi->{SOURCE},
                                                      "SPI_FUNCTION");
            if ($function eq "PNOR") {
                $spiMaster{$flashName}{label} = "pnor";
                $foundPNOR = 1;
            }

            $chipSelect++;
        }
    }

    return (\%spiMaster, $foundPNOR);
}


#Returns a hash that represents the mbox node.
#This node is used by the LPC mailbox device driver.
#Only present if the LPC mailbox is enabled in the config file.
#Node looks like:
#  &mbox {
#    status = "okay";
#  }
sub getMBoxNode
{
    my %node;
    if (exists $g_configuration{"lpc-host-spi-flash-mailbox"}) {
        statusOK(\%node);
    }

    return %node;
}


#Returns a hash that represents the LPC node.
#Only present if the LPC mailbox is enabled in the config file.
#Node looks like:
#  &lpc_ctrl {
#     flash = <&spi1>;
#     memory-region = <&flash_memory>;
#     status = "okay";
#};
sub getLPCNode
{
    my %node;
    if (exists $g_configuration{"lpc-host-spi-flash-mailbox"}) {

        statusOK(\%node);

        #Point to the reserved-memory region label
        $node{"memory-region"} = "<(ref)" .
                                 HOST_SPI_FLASH_MEM_REGION_NODE_LABEL . ">";

        if (not defined $g_pnorNodeName) {
            die "The PNOR SPI flash node cannot be found but is required " .
                "if the LPC mailbox is enabled.\n";
        }

        $node{flash} = "<(ref)$g_pnorNodeName>";
    }

    return %node;
}


#Returns a hash that represents the leds node by finding all of the
#GPIO connections to LEDs.
#Node will look like:
#   leds {
#       <ledname> {
#          gpios =  &gpio ASPEED_GPIO(x, y) GPIO_ACTIVE_xxx>
#       };
#       <another ledname> {
#       ...
#   }
sub getLEDNode
{
    my %leds;

    $leds{compatible} = "gpio-leds";

    my $connections = $g_targetObj->findConnections($g_bmc, "GPIO", "LED");

    if ($connections eq "") {
        print "WARNING:  No LEDs found connected to the BMC\n";
        return %leds;
    }

    foreach my $gpio (@{$connections->{CONN}}) {
        my %ledNode;

        $ledNode{COMMENT} = connectionComment($gpio);

        #The node name will be the simplified LED name
        my $name = $gpio->{DEST_PARENT};
        $name =~ s/(-\d+$)//; #remove trailing position
        $name =~ s/.*\///;    #remove the front of the path

        #For now only supports ASPEED.
        if (uc($g_bmcMfgr) ne "ASPEED") {
            die "ERROR:  Unsupported BMC manufacturer $g_bmcMfgr\n";
        }
        my $num = $g_targetObj->getAttribute($gpio->{SOURCE}, "PIN_NUM");
        my $macro = getAspeedGpioMacro($num);

        #If it's active high or low
        my $state = $g_targetObj->getAttribute($gpio->{DEST_PARENT}, "ON_STATE");
        my $activeString = getGpioActiveString($state);

        $ledNode{gpios} = "<&gpio $macro $activeString>";

        $leds{$name} = { %ledNode };
    }

    return %leds;
}


#Returns a either GPIO_ACTIVE_HIGH or GPIO_ACTIVE_LOW
#  $val = either a 1 or a 0 for active high or low
sub getGpioActiveString
{
    my $val = shift;

    if ($val == 0) {
       return "GPIO_ACTIVE_LOW";
    }

    return "GPIO_ACTIVE_HIGH";
}


#Turns a GPIO number into something like ASPEED_GPIO(A, 0) for the
#ASPEED GPIO numbering scheme A[0-7] -> Z[0-7] and then starts at
#AA[0-7] after that.
#  $num = the GPIO number
sub getAspeedGpioMacro
{
    my $num = shift;
    my $char;
    my $offset = $num % 8;
    my $block = int($num / 8);

    #If past Z, wraps to AA, AB, etc
    if ((ord('A') + $block) > ord('Z')) {
        #how far past Z?
        $char = $block - (ord('Z') - ord('A'));

        #Don't let it wrap twice
        if ($char > (ord('Z') - ord('A') + 1)) {
            die "ERROR: Invalid PIN_NUM value $num found for GPIO\n";
        }

        #start back at 'A' again, and convert to a character
        $char = chr($char + ord('A') - 1);

        #Add in a bonus 'A', to get something like AB
        $char = "A".$char;
    }
    else {
        $char = ord('A') + $block;
        $char = chr($char);
    }

    return "ASPEED_GPIO($char, $offset)";
}


#Returns a hash that represents the OpenFSI device tree node.
#This node defines the GPIOs used by FSI.
#Node will look like:
#  fsi-master {
#    status = "okay";
#    compatible = "ibm,fsi-master-gpio", "ibm,fsi-master";
#    clock-gpios = <&gpio ASPEED_GPIO(AA, 0) GPIO_ACTIVE_HIGH>;
#    data-gpios = <&gpio ASPEED_GPIO(E, 0) GPIO_ACTIVE_HIGH>;
#    enable-gpios = <&gpio ASPEED_GPIO(D, 0) GPIO_ACTIVE_HIGH>;
#    mux-gpios = <&gpio ASPEED_GPIO(A, 6) GPIO_ACTIVE_HIGH>;
#    trans-gpios = <&gpio ASPEED_GPIO(R, 2) GPIO_ACTIVE_HIGH>;
#  };
sub getFSINode
{
    my %node;
    my $enabled = 0;

    #For now only supports ASPEED because of the GPIO syntax.
    if (uc($g_bmcMfgr) ne "ASPEED") {
        die "ERROR:  Unsupported BMC manufacturer $g_bmcMfgr in getFSINode\n";
    }

    #Check that OpenFSI is enabled in the config file.
    if (exists $g_configuration{"enable-openfsi"}) {
        if ($g_configuration{"enable-openfsi"} eq "true") {
            $enabled = 1;
        }
        elsif (($g_configuration{"enable-openfsi"} ne "true") &&
               ($g_configuration{"enable-openfsi"} ne "false")) {
            die "Invalid enable-openfsi config file value: " .
                $g_configuration{"enable-openfsi"} . "\n";
        }
    }

    return %node unless ($enabled == 1);

    #In the MRW there is an fsi_bit_bang logical part that connects to the
    #BMC's 5 FSI related GPIOs.  This part then has an FSI master unit that
    #would connect to the P9's FSI slave (though we don't check that).

    #Find the specific GPIOs by the fsi_bit_bang slave
    #unit that they're connected to.  Some are optional.
    my %fsiGpios = ("fsi_bit_bang.fsi_clk" =>
                    {
                        name => "clock-gpios",
                        gpio => undef,
                        required => 1
                    },
                    "fsi_bit_bang.fsi_dat" =>
                    {
                        name => "data-gpios",
                        gpio => undef,
                        required => 1
                    },
                    "fsi_bit_bang.fsi_mux" =>
                    {
                        name => "mux-gpios",
                        gpio => undef
                    },
                    "fsi_bit_bang.fsi_enable" =>
                    {
                        name => "enable-gpios",
                        gpio => undef
                    },
                    "fsi_bit_bang.fsi_trans" =>
                    {
                        name => "trans-gpios",
                        gpio => undef
                    }
                   );

    my $connections = $g_targetObj->findConnections($g_bmc, "GPIO");
    if ($connections eq "") {
        die "No GPIO connections found in getFSINode\n";
    }

    for my $gpio (@{$connections->{CONN}}) {

        #Check if the destination's slave unit is in our list.
        my $slaveUnit = $g_targetObj->getInstanceName($gpio->{DEST});

        if (exists $fsiGpios{$slaveUnit}) {
            my $num = $g_targetObj->getAttribute($gpio->{SOURCE}, "PIN_NUM");
            $fsiGpios{$slaveUnit}{gpio} = getAspeedGpioMacro($num);
        }
    }

    statusOK(\%node);

    push @{$node{compatible}}, "ibm,fsi-master", "ibm,fsi-master-gpio";

    while (my ($key, $hash) = each(%fsiGpios)) {
        if (not defined $hash->{gpio}) {
            if (exists $hash->{required}) {
                die "Missing connection for FSI GPIO $key\n";
            }
        }
        else {
            $node{$hash->{name}} = "<&gpio $hash->{gpio} GPIO_ACTIVE_HIGH>";
        }
    }

    return %node;
}


#Returns a list of hashes that represent the UART nodes on the BMC by
#finding the UART connections.
#Nodes will look like:
#  &uartX {
#     status = "okay"
#  }
sub getUARTNodes
{
    my @nodes;

    #Using U750 for legacy MRW reasons
    my $connections = $g_targetObj->findConnections($g_bmc, "U750");

    if ($connections eq "") {
        print "WARNING:  No UART buses found connected to the BMC\n";
        return @nodes;
    }

    foreach my $uart (@{$connections->{CONN}}) {
        my %node;

        my $num = $g_targetObj->getAttribute($uart->{SOURCE}, "CHIP_UNIT");
        my $name = "uart$num";

        statusOK(\%{$node{$name}});
        $node{$name}{COMMENT} = connectionComment($uart);

        #Add in any pinctrl properties.  These would come from the parent
        #of $uart{SOURCE}, which would be a unit-pingroup-bmc if the
        #pins for this connection are multi-function.
        addPinCtrlProps($g_targetObj->getTargetParent($uart->{SOURCE}),
                        \%{$node{$name}});

        push @nodes, { %node };
    }

    return @nodes;
}


#Returns a list of hashes that represent the MAC (ethernet) nodes on the BMC
#by finding the connections of type ETHERNET.
#Nodes will look like:
#  &macX {
#    ...
#  }
sub getMacNodes
{
    my @nodes;

    my $connections = $g_targetObj->findConnections($g_bmc, "ETHERNET");

    if ($connections eq "") {
        print "WARNING:  No ethernet buses found connected to the BMC\n";
        return @nodes;
    }

    foreach my $eth (@{$connections->{CONN}}) {
        my %node;

        my $num = $g_targetObj->getAttribute($eth->{SOURCE}, "CHIP_UNIT");
        my $ncsi = $g_targetObj->getAttribute($eth->{SOURCE}, "NCSI_MODE");
        my $hwChecksum = $g_targetObj->getAttribute($eth->{SOURCE},
                                                  "USE_HW_CHECKSUM");

        my $name = "mac$num";
        statusOK(\%{$node{$name}});

        if ($ncsi == 1) {
            $node{$name}{"use-ncsi"} = ZERO_LENGTH_PROPERTY;
        }
        if ($hwChecksum == 0) {
            $node{$name}{"no-hw-checksum"} = ZERO_LENGTH_PROPERTY;
        }

        $node{$name}{COMMENT} = connectionComment($eth);

        #Add in any pinctrl properties.  These would come from the parent
        #of $eth{SOURCE}, which would be a unit-pingroup-bmc if the
        #pins for this connection are multi-function.
        addPinCtrlProps($g_targetObj->getTargetParent($eth->{SOURCE}),
                        \%{$node{$name}});

        push @nodes, { %node };
    }

    return @nodes;
}


#Returns a list of hashes that represent the virtual UART nodes
#Node will look like:
#  &vuart {
#   status = "okay"
#  }
sub getVuartNodes
{
    my @nodes;
    my %node;

    #For now, enable 1 node all the time.
    #TBD if this needs to be fixed
    statusOK(\%{$node{vuart}});

    push @nodes, { %node };

    return @nodes;
}

#Returns a list of hashes that represent the I2C device nodes.
#There is 1 parent node for each bus, which then have subnodes
#for each device on that bus.  If a bus doesn't have any
#attached devices, it doesn't need to show up.
#The nodes will look like:
#  &i2c0 {
#     status = "okay"
#     device1@addr { (addr = 7 bit I2C address)
#       reg = <addr>
#       compatible = ...
#       ...
#     }
#     device2@addr {
#       reg = <addr>
#       ...
#     }
#  }
#  &i2c1 {
#  ...
#  }
sub getI2CNodes
{
    my @nodes;
    my %busNodes;

    my $connections = $g_targetObj->findConnections($g_bmc, "I2C");

    if ($connections eq "") {
        print "WARNING:  No I2C buses found connected to the BMC\n";
        return @nodes;
    }

    foreach my $i2c (@{$connections->{CONN}}) {

        my %deviceNode, my $deviceName;

        $deviceNode{COMMENT} = connectionComment($i2c);

        $deviceName = lc $i2c->{DEST_PARENT};
        $deviceName =~ s/-\d+$//; #remove trailing position
        $deviceName =~ s/.*\///;  #remove the front of the path

        #Get the I2C address
        my $i2cAddress = $g_targetObj->getAttribute($i2c->{DEST}, "I2C_ADDRESS");
        $i2cAddress = hex($i2cAddress);
        if ($i2cAddress == 0) {
            die "ERROR: Missing I2C address on $i2c->{DEST}\n";
        }

        #Put it in the format we want to print it in
        $i2cAddress = Util::adjustI2CAddress($i2cAddress);
        addRegProp(\%deviceNode, $i2cAddress);

        $deviceName = makeNodeName($deviceName, $deviceNode{reg});

        #Get the I2C bus number
        if ($g_targetObj->isBadAttribute($i2c->{SOURCE},
                                         "I2C_PORT")) {
            die "ERROR: I2C_PORT attribute in $i2c->{DEST_PARENT} " .
                "is either missing or empty.\n";
        }

        my $busNum = $g_targetObj->getAttribute($i2c->{SOURCE}, "I2C_PORT");
        if ($busNum =~ /0x/i) {
            $busNum = hex($busNum);
        }

        #Convert the number to the Linux numbering scheme.
        $busNum += $g_i2cBusAdjust;

        #Get the compatible property
        if ($g_targetObj->isBadAttribute($i2c->{DEST_PARENT},
                                         "BMC_DT_COMPATIBLE")) {
            die "ERROR: BMC_DT_COMPATIBLE attribute in $i2c->{DEST_PARENT} " .
                "is either missing or empty.\n";
        }

        $deviceNode{compatible} = $g_targetObj->getAttribute(
                                                    $i2c->{DEST_PARENT},
                                                    "BMC_DT_COMPATIBLE");

        #Get any other part specific properties, where the property
        #names are actually defined in the XML.
        my %props = getPartDefinedDTProperties($i2c->{DEST_PARENT});
        foreach my $prop (sort keys %props) {
            $deviceNode{$prop} = $props{$prop};
        }

        #busNodeName is the hash twice so when we loop
        #below it doesn't get lost
        my $busNodeName = "i2c$busNum";
        statusOK(\%{$busNodes{$busNodeName}{$busNodeName}});
        $busNodes{$busNodeName}{$busNodeName}{$deviceName} = { %deviceNode };

        #Add in any pinctrl properties.  These would come from the parent
        #of $i2c{SOURCE}, which would be a unit-pingroup-bmc if the
        #pins for this connection are multi-function.
        addPinCtrlProps($g_targetObj->getTargetParent($i2c->{SOURCE}),
                        \%{$busNodes{$busNodeName}{$busNodeName}});
    }

    #Each bus gets its own hash entry in the array
    for my $b (sort keys %busNodes) {
        push @nodes, { %{$busNodes{$b}} };
    }

    return @nodes;
}


#Returns a hash of property names and values that should be stored in
#the device tree node for this device. The names of the properties and
#the attributes to find their values in are stored in the
#BMC_DT_ATTR_NAMES attribute in the chip.
#  $chip = the chip target
sub getPartDefinedDTProperties
{
    my $chip = shift;
    my %props;

    if ($g_targetObj->isBadAttribute($chip, "BMC_DT_ATTR_NAMES")) {
        return %props;
    }

    my $attr = $g_targetObj->getAttribute($chip, "BMC_DT_ATTR_NAMES");
    $attr =~ s/\s//g;
    my @names = split(',', $attr);

    #There can be up to 4 entries in this attribute
    for (my $i = 0; $i < scalar @names; $i += 2) {

        #$names[$i] holds the name of the attribute.
        #$names[$i+1] holds the name of the property to store its value in.
        if (($names[$i] ne "NA") && ($names[$i] ne "")) {

            my $val = $g_targetObj->getAttribute($chip, $names[$i]);

            #if the value is empty, assume it's for a standalone property,
            #which gets turned into: some-property;
            if ($val eq "") {
                $props{$names[$i+1]} = ZERO_LENGTH_PROPERTY;
            }
            else {
                $props{$names[$i+1]} = "<$val>";
            }
        }
    }

    return %props;
}


#Sets the global $g_i2cBusAdjust from the configuration file.
sub getI2CBusAdjust
{
    if (exists $g_configuration{"i2c-bus-adjust"}) {

        $g_i2cBusAdjust = $g_configuration{"i2c-bus-adjust"};

        if (!looks_like_number($g_i2cBusAdjust)) {
            die "ERROR:  Invalid i2c-bus-adjust value $g_i2cBusAdjust " .
                "found in config file.\n";
        }
    }
    else {
        $g_i2cBusAdjust = 0;
        print "WARNING: No I2C Bus number adjustment done " .
              "for this system.\n";
    }
}



#Adds two pinctrl properties to the device node hash passed in,
#if specified in the MRW.  Pin Control refers to a mechanism for
#Linux to know which function of a multi-function pin to configure.
#For example, a pin could either be configured to be a GPIO, or
#an I2C clock line.  The pin function depends on board wiring,
#so is known by the MRW.
#  $target = the target to get the BMC_DT_PINCTRL_FUNCTS attribute from
#  $node = a hash reference to the device tree node to add the properties to
sub addPinCtrlProps
{
    my ($target, $node) = @_;

    if (!$g_targetObj->isBadAttribute($target, "BMC_DT_PINCTRL_FUNCS")) {
        my $attr = $g_targetObj->getAttribute($target,
                                              "BMC_DT_PINCTRL_FUNCS");

        my $pinCtrl0Prop = makePinCtrl0PropValue($attr);
        if ($pinCtrl0Prop ne "") {
            $node->{"pinctrl-names"} = "default";
            $node->{"pinctrl-0"} = $pinCtrl0Prop;
        }
    }
}


#Constructs the pinctrl-0 property value based on the
#BMC_DT_PINCTRL_FUNCS attribute passed in.
#  $attr = BMC_DT_PINCTRL_FUNCS attribute value, which is an array
sub makePinCtrl0PropValue
{
    my $attr = shift;
    my @entries;
    my $value = "";

    $attr =~ s/\s//g;
    my @funcs = split(',', $attr);
    foreach my $func (@funcs) {
        if (($func ne "NA") && ($func ne "")) {
            push @entries, $func;
        }
    }

    #<&pinctrl_funcA_default &pinctrl_funcB_default ...>
    if (scalar @entries) {
        $value = "<";
        foreach my $entry (@entries) {
            $value .= "&pinctrl_".$entry."_default ";
        }
        $value =~ s/\s$//; #Remove the trailing space
        $value .= ">";
    }

    return $value;
}


#Returns a list of compatible fields for the BMC itself.
sub getBMCCompatibles
{
    my @compats;

    #1st entry:  <system mfgr>,<system name>-bmc
    #2nd entry:  <bmc mfgr>,<bmc model>

    foreach my $target (sort keys %{ $g_targetObj->getAllTargets() }) {
        if ($g_targetObj->getType($target) eq "SYS") {
           my $mfgr = $g_targetObj->getAttribute($target, "MANUFACTURER");
           push @compats, lc "$mfgr,$g_systemName-bmc";
           last;
        }
    }

    push @compats, lc($g_bmcMfgr).",".lc($g_bmcModel);

    return @compats;
}


#Returns a string for the system's BMC model property
sub getSystemBMCModel
{
    #'<System> BMC'
    my $sys = lc $g_systemName;
    $sys = uc(substr($sys, 0, 1)) . substr($sys, 1);

    return $sys . " BMC";
}

#Create the comment that will show up in the device tree
#for a connection.  In the output, will look like:
# // sourceUnit ->
# // destChip
#
#  $conn = The connection hash reference
sub connectionComment
{
    my $conn = shift;
    my $comment = "$conn->{SOURCE} ->\n$conn->{DEST_PARENT}";
    return $comment;
}


#Prints a list of nodes at the same indent level
#  $f = file handle
#  $level = indent level (0,1,etc)
#  @nodes = array of node hashes to print, where the
#  key for the hash is the name of the node
sub printNodes
{
    my ($f, $level, @nodes) = @_;

    foreach my $n (@nodes) {
        my %node = %$n;

        foreach my $name (sort keys %node) {
            my %n = %{ $node{$name} };
            printNode($f, $level, $name, %n);
        }
    }
}


#Print a single node and its children
#  $f = file handle
#  $level = indent level (0,1,etc)
#  $name = the name of the node - shows up as:
#     name { ...
#  %vals = The contents of the node, with the following options:
#     if the key is:
#     - 'DTSI_INCLUDE', then value gets turned into a #include
#     - 'COMMENT', then value gets turned into a // comment
#     - 'ZERO_LENGTH_PROPERTY' then value gets turned into:  value;
#
#     If the value is:
#     - a hash - then that hash gets turned into a child node
#       where the key is the name of the child node
#     - an array - will print a property list, like: "a", "b"
sub printNode
{
    my ($f, $level, $name, %vals) = @_;
    my $include = "";

    #No reason to print an empty node
    if (!keys %vals) {
        return;
    }

    if ($level == 0) {
        $name = "&".$name;
    }

    print $f "\n";

    if (exists $vals{COMMENT}) {
        my @lines = split('\n', $vals{COMMENT});
        foreach my $l (@lines) {
            print $f indent($level) . "// $l\n";
        }
    }

    #The node can have a label, which looks like:
    #label : name {
    my $label = "";
    if (exists $vals{NODE_LABEL}) {
        $label = $vals{NODE_LABEL} . ": ";
    }

    print $f indent($level) . $label . "$name {\n";

    #First print properties, then includes, then subnodes

    #Print Properties
    foreach my $v (sort keys %vals) {

        next if ($v eq "COMMENT");
        next if ($v eq "DTSI_INCLUDE");
        next if ($v eq "NODE_LABEL");
        next if (ref($vals{$v}) eq "HASH");

        if (ref($vals{$v}) eq "ARRAY") {
            printPropertyList($f, $level+1, $v, @{$vals{$v}});
        }
        elsif ($vals{$v} ne ZERO_LENGTH_PROPERTY) {
            printProperty($f, $level+1, $v, $vals{$v});
        }
        else {
            printZeroLengthProperty($f, $level+1, $v);
        }
    }

    #Print Includes
    foreach my $v (sort keys %vals) {

        if ($v eq "DTSI_INCLUDE") {
            #print 1 include per line
            my @incs = split(',', $vals{$v});
            foreach my $i (@incs) {
                print $f qq(#include "$i"\n);
            }
        }
    }

    #Print Nodes
    foreach my $v (sort keys %vals) {

        if (ref($vals{$v}) eq "HASH") {
            printNode($f, $level+1, $v, %{$vals{$v}});
        }
    }

    print $f indent($level) . "};\n";
}


#Prints a comma separated list of properties.
#e.g.  a = "b, c, d";
#  $f = file handle
#  $level = indent level (0,1,etc)
#  $name = name of property
#  @vals = list of property values
sub printPropertyList
{
    my ($f, $level, $name, @vals) = @_;

    print $f indent($level) . "$name = ";

    for (my $i = 0;$i < scalar @vals; $i++) {
        print $f qq("$vals[$i]");
        if ($i < (scalar(@vals) - 1)) {
            print $f ", ";
        }
    }
    print $f ";\n"
}


#Prints a single property.  e.g. a = "b";
#  $f = file handle
#  $level = indent level (0,1,etc)
#  $name = name of property
#  @vals = property values
sub printProperty
{
    my ($f, $level, $name, $val) = @_;
    my $quoteChar = qq(");

    $val = convertReference($val);

    #properties with < > or single word aliases don't need quotes
    if (($val =~ /<.*>/) || ($val =~ /^&\w+$/)) {
        $quoteChar = "";
    }

    print $f indent($level) . "$name = $quoteChar$val$quoteChar;\n";
}


#Prints a zero length property e.g. some-property;
#  $f = file handle
#  $level = indent level (0,1,etc)
#  $name = name of property
sub printZeroLengthProperty
{
    my ($f, $level, $name) = @_;
    print $f indent($level) . "$name;\n";
}


#Replace '(ref)' with '&'.
#Needed because Serverwiz doesn't properly escape '&'s in the XML,
#so the '(ref)' string is used to represent the reference
#specifier instead of '&'.
sub convertReference
{
    my $val = shift;
    $val =~ s/\(ref\)/&/g;
    return $val
}


#Prints the device tree version line.
#  $f = file handle
sub printVersion
{
    my $f = shift;
    print $f VERSION."\n"
}


#Prints the #include line for pulling in an include file.
#The files to include come from the configuration file.
#  $f = file handle
#  $type = include type
sub printIncludes
{
    my ($f, $type) = @_;
    my @includes = getIncludes($type);

    foreach my $i (@includes) {
        #if a .dtsi, gets " ", otherwise < >
        if ($i =~ /\.dtsi$/) {
            $i = qq("$i");
        }
        else {
            $i = "<$i>";
        }
        print $f "#include $i\n";
    }
}


#Returns an array of include files found in the config file
#for the type specified.
# $type = the include type, which is the section name in the
#         YAML configuration file.
sub getIncludes
{
    my $type = shift;
    my @includes;

    #The config file may have a section but no includes
    #listed in it, which is OK.
    if ((exists $g_configuration{includes}{$type}) &&
        (ref($g_configuration{includes}{$type}) eq "ARRAY")) {

        @includes = @{$g_configuration{includes}{$type}};
    }

    return @includes;
}


#Appends the first value of the 'reg' property
#passed in to the name passed in to create the
#full name for the node
#  $name = node name that will be appended to
#  $reg = the reg property values
sub makeNodeName
{
    my ($name, $reg) = @_;

    $reg =~ s/<//g;
    $reg =~ s/>//g;
    my @vals = split(' ', $reg);

    if (scalar @vals > 0) {
        $vals[0] =~ s/0x//;
        $name .= "@" . lc $vals[0];
    }

    return $name;
}


#Prints the root node starting bracket.
#  $f = file handle
sub printRootNodeStart
{
    my $f = shift;
    print $f qq(/ {\n);
}


#Prints the root node ending bracket.
#  $f = file handle
#  $level = indent level (0,1,etc)
sub printRootNodeEnd
{
    my ($f, $level) = @_;
    print $f indent($level).qq(};\n);
}


#Returns a string that can be used to indent based on the
#level passed in.  Each level is an additional 4 spaces.
#  $level = indent level (0,1,etc)
sub indent
{
    my $level = shift;
    return ' ' x ($level * 4);
}


#Adds a {status} = "okay" element to the hash passed in.
# $node = reference to the hash to add element to
sub statusOK
{
    my $node = shift;
    $node->{status} = "okay";
}


#Adds the {reg} element to the hash passed in using the values
#passed in.  Resulting value looks like: "<val1  val2  etc>"
# $node = reference to the hash to add element to
# @values = the values for the property.  May be passed in one at
#           a time and not as an array.
sub addRegProp
{
    my $node = shift;
    my @values = @_;

    $node->{reg} = "<";
    for (my $i = 0; $i < scalar @values; $i++) {
        $node->{reg} .= $values[$i];
        if ($i < (scalar @values) - 1) {
            $node->{reg} .= " ";
        }
    }
    $node->{reg} .= ">";
}


sub printUsage
{
    print "gen_devtree.pl -x [XML filename] -y [yaml config file] " .
          "-o [output filename]\n";
    exit(1);
}
