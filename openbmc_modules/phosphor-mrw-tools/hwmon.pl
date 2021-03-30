#!/usr/bin/env perl

#Creates a configuration file for each hwmon sensor in the MRW
#for use by the phosphor-hwmon daemon.  These configuration files
#contain labels and thresholds for the hwmon features for that sensor.
#The files are created in subdirectories based on their device
#tree paths.

use strict;
use warnings;

use mrw::Targets;
use mrw::Util;
use Getopt::Long;
use File::Path qw(make_path);

use constant {
    I2C_TYPE => "i2c"
};

my $serverwizFile;
my $g_outputDir;
my @hwmon;

GetOptions("x=s" => \$serverwizFile,
           "d=s" => \$g_outputDir) or printUsage();

if (not defined $serverwizFile) {
    printUsage();
}

my $g_targetObj = Targets->new;
$g_targetObj->loadXML($serverwizFile);

my $bmc = Util::getBMCTarget($g_targetObj);

getI2CSensors($bmc, \@hwmon);

makeConfFiles($bmc, \@hwmon);

exit 0;


#Returns an array of hashes that represent hwmon enabled I2C sensors.
sub getI2CSensors
{
    my ($bmc, $hwmon) = @_;
    my $connections = $g_targetObj->findConnections($bmc, "I2C");

    return if ($connections eq "");

    for my $i2c (@{$connections->{CONN}}) {

        my $chip = $i2c->{DEST_PARENT};
        my @hwmonUnits = Util::getChildUnitsWithTargetType($g_targetObj,
                                                      "unit-hwmon-feature",
                                                      $chip);

        #If the MRW doesn't specify a label for a particular hwmon
        #feature, then we don't want to use it.
        removeUnusedHwmons(\@hwmonUnits);

        #If chip didn't have hwmon units, it isn't hwmon enabled.
        next unless (scalar @hwmonUnits > 0);

        my %entry;
        $entry{type} = I2C_TYPE;
        $entry{name} = lc $g_targetObj->getInstanceName($chip);
        getHwmonAttributes(\@hwmonUnits, \%entry);
        getI2CAttributes($i2c, \%entry);

        push @$hwmon, { %entry };
    }
}


#Removes entries from the list of hwmon units passed in that have
#an empty HWMON_NAME or DESCRIPTIVE_NAME attribute.
sub removeUnusedHwmons
{
    my ($units) = @_;
    my $i = 0;

    while ($i <= $#$units) {

        my $hwmon = $g_targetObj->getAttributeField($$units[$i],
                                                    "HWMON_FEATURE",
                                                    "HWMON_NAME");
        my $name = $g_targetObj->getAttributeField($$units[$i],
                                                   "HWMON_FEATURE",
                                                   "DESCRIPTIVE_NAME");
        if (($hwmon eq "") || ($name eq "")) {
            splice(@$units, $i, 1);
        }
        else {
            $i++;
        }
    }
}


#Reads the hwmon related attributes from the HWMON_FEATURE
#complex attribute and adds them to the hash.
sub getHwmonAttributes
{
    my ($units, $entry) = @_;
    my %hwmonFeatures;

    for my $unit (@$units) {

        #The hwmon name, like 'in1', 'temp1', 'fan1', etc
        my $hwmon = $g_targetObj->getAttributeField($unit,
                                                    "HWMON_FEATURE",
                                                    "HWMON_NAME");

        #The useful name for this feature, like 'ambient'
        my $name = $g_targetObj->getAttributeField($unit,
                                                   "HWMON_FEATURE",
                                                   "DESCRIPTIVE_NAME");
        $hwmonFeatures{$hwmon}{label} = $name;

        #Thresholds are optional, ignore if NA
        my $warnHigh = $g_targetObj->getAttributeField($unit,
                                                       "HWMON_FEATURE",
                                                       "WARN_HIGH");
        if (($warnHigh ne "") && ($warnHigh ne "NA")) {
            $hwmonFeatures{$hwmon}{warnhigh} = $warnHigh;
        }

        my $warnLow = $g_targetObj->getAttributeField($unit,
                                                      "HWMON_FEATURE",
                                                      "WARN_LOW");
        if (($warnLow ne "") && ($warnLow ne "NA")) {
            $hwmonFeatures{$hwmon}{warnlow} = $warnLow;
        }

        my $critHigh = $g_targetObj->getAttributeField($unit,
                                                       "HWMON_FEATURE",
                                                       "CRIT_HIGH");
        if (($critHigh ne "") && ($critHigh ne "NA")) {
            $hwmonFeatures{$hwmon}{crithigh} = $critHigh;
        }

        my $critLow = $g_targetObj->getAttributeField($unit,
                                                      "HWMON_FEATURE",
                                                      "CRIT_LOW");
        if (($critLow ne "") && ($critHigh ne "NA")) {
            $hwmonFeatures{$hwmon}{critlow} = $critLow;
        }
    }

    $entry->{hwmon} = { %hwmonFeatures };
}


#Reads the I2C attributes for the chip and adds them to the hash.
#This includes the i2C address, and register base address and
#offset for the I2C bus the chip is on.
sub getI2CAttributes
{
    my ($i2c, $entry) = @_;

    #The address comes from the destination unit, and needs
    #to be the 7 bit value in hex without the 0x.
    my $addr = $g_targetObj->getAttribute($i2c->{DEST}, "I2C_ADDRESS");
    $addr = hex($addr) >> 1;
    $entry->{addr} = sprintf("%x", $addr);

    #The reg base address and offset may be optional depending on
    #the BMC chip type.  We'll check later if it's required but missing.
    if (!$g_targetObj->isBadAttribute($i2c->{SOURCE}, "REG_BASE_ADDRESS")) {
        my $addr = $g_targetObj->getAttribute($i2c->{SOURCE},
                                              "REG_BASE_ADDRESS");
        $entry->{regBaseAddress} = sprintf("%x", hex($addr));
    }

    if (!$g_targetObj->isBadAttribute($i2c->{SOURCE}, "REG_OFFSET")) {
        my $offset = $g_targetObj->getAttribute($i2c->{SOURCE},
                                                "REG_OFFSET");
        $entry->{regOffset} = sprintf("%x", hex($offset));
    }
}


#Creates .conf files for each chip.
sub makeConfFiles
{
    my ($bmc, $hwmon) = @_;

    for my $entry (@$hwmon) {
        printConfFile($bmc, $entry);
    }
}


#Writes out a configuration file for a hwmon sensor, containing:
#  LABEL_<feature> = <descriptive label>  (e.g. LABEL_temp1 = ambient)
#  WARNHI_<feature> = <value> (e.g. WARNHI_temp1 = 99)
#  WARNLO_<feature> = <value> (e.g. WARNLO_temp1 = 0)
#  CRITHI_<feature> = <value> (e.g. CRITHI_temp1 = 100)
#  CRITHI_<feature> = <value> (e.g. CRITLO_temp1 = -1)
#
#  The file is created in a subdirectory based on the chip's device
#  tree path.
sub printConfFile
{
    my ($bmc, $entry) = @_;
    my $path = getConfFilePath($bmc, $entry);
    my $name = $path . "/" . getConfFileName($entry);

    make_path($path);

    open(my $f, ">$name") or die "Could not open $name\n";

    for my $feature (sort keys %{$entry->{hwmon}}) {
        print $f "LABEL_$feature = \"$entry->{hwmon}{$feature}{label}\"\n";

        #Thresholds are optional
        if (exists $entry->{hwmon}{$feature}{warnhigh}) {
            print $f "WARNHI_$feature = \"$entry->{hwmon}{$feature}{warnhigh}\"\n";
        }
        if (exists $entry->{hwmon}{$feature}{warnlow}) {
            print $f "WARNLO_$feature = \"$entry->{hwmon}{$feature}{warnlow}\"\n";
        }
        if (exists $entry->{hwmon}{$feature}{crithigh}) {
            print $f "CRITHI_$feature = \"$entry->{hwmon}{$feature}{crithigh}\"\n";
        }
        if (exists $entry->{hwmon}{$feature}{critlow}) {
            print $f "CRITLO_$feature = \"$entry->{hwmon}{$feature}{critlow}\"\n";
        }
    }

    close $f;
}


#Returns the chip's configuration file path.
sub getConfFilePath
{
    my ($bmc, $entry) = @_;

    my $mfgr = $g_targetObj->getAttribute($bmc, "MANUFACTURER");

    #Unfortunately, because the conf file path is based on the
    #device tree path which is tied to the internal chip structure,
    #this has to be model specific.  Until proven wrong, I'm going
    #to make an assumption that all ASPEED chips have the same path
    #as so far all of the models I've seen do.
    if ($mfgr eq "ASPEED") {
        return getAspeedConfFilePath($entry);
    }
    else {
        die "Unsupported BMC manufacturer $mfgr\n";
    }
}


#Returns the relative path of the configuration file to create.
#This path is based on the path of the chip in the device tree.
#An example path is  ahb/apb/i2c@1e78a000/i2c-bus@400/
sub getAspeedConfFilePath
{
    my ($entry) = @_;
    my $path;

    if ($entry->{type} eq I2C_TYPE) {

        #ASPEED requires the reg base address & offset fields
        if ((not exists $entry->{regBaseAddress}) ||
            (not exists $entry->{regOffset})) {
            die "Missing regBaseAddress or regOffset attributes " .
                "in the I2C master unit XML\n";
        }

        $path = "$g_outputDir/ahb/apb/i2c\@$entry->{regBaseAddress}/i2c-bus@" .
                "$entry->{regOffset}";
    }
    else {
        #TODO: FSI support for the OCC when known
        die "HWMON bus type $entry->{type} not implemented yet\n";
    }

    return $path;
}


#Returns the name to use for the conf file:
#  <name>@<addr>.conf  (e.g. rtc@68.conf)
sub getConfFileName
{
    my ($entry) = @_;
    return "$entry->{name}\@$entry->{addr}.conf";
}


sub printUsage
{
    print "$0 -x [XML filename] -d [output base directory]\n";
    exit(1);
}
