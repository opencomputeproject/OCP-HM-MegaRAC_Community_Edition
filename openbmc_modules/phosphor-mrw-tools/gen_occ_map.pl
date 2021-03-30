#!/usr/bin/env perl
use strict;
use warnings;

use mrw::Targets; # Set of APIs allowing access to parsed ServerWiz2 XML output
use mrw::Inventory; # To get list of Inventory targets
use Getopt::Long; # For parsing command line arguments

# Globals
my $force           = 0;
my $serverwizFile  = "";
my $debug           = 0;
my $outputFile     = "";
my $verbose         = 0;

# Command line argument parsing
GetOptions(
"f"   => \$force,             # numeric
"i=s" => \$serverwizFile,    # string
"o=s" => \$outputFile,       # string
"d"   => \$debug,
"v"   => \$verbose,
)
or printUsage();

if (($serverwizFile eq "") or ($outputFile eq ""))
{
    printUsage();
}

# Hashmap of all the OCCs and their sensor IDs
my %occHash;

# API used to access parsed XML data
my $targetObj = Targets->new;
if($verbose == 1)
{
    $targetObj->{debug} = 1;
}

if($force == 1)
{
    $targetObj->{force} = 1;
}

$targetObj->loadXML($serverwizFile);
print "Loaded MRW XML: $serverwizFile \n";

# Process all the targets in the XML
foreach my $target (sort keys %{$targetObj->getAllTargets()})
{
    # Only take the instances having 'OCC" as TYPE
    if ("OCC" ne $targetObj->getAttribute($target, "TYPE"))
    {
        next;
    }

    # OCC instance and sensor ID to insert into output file
    my $instance = "";
    my $sensor = "";

    # Now that we are in OCC target instance, get the instance number
    $instance = $targetObj->getAttribute($target, "IPMI_INSTANCE");

    # Each OCC would have occ_active_sensor child that would have
    # more information, such as Sensor ID.
    # This would be an array of children targets
    my $children = $targetObj->getTargetChildren($target);

    for my $child (@{$children})
    {
        $sensor = $targetObj->getAttribute($child, "IPMI_SENSOR_ID");
    }

    # Populate a hashmap with OCC and its sensor ID
    $occHash{$instance} = $sensor;

} # All the targets

# Generate the yaml file
generateYamlFile();
##------------------------------------END OF MAIN-----------------------

sub generateYamlFile
{
    my $fileName = $outputFile;
    open(my $fh, '>', $fileName) or die "Could not open file '$fileName' $!";

    foreach my $instance (sort keys %occHash)
    {
        # YAML with list of {Instance:SensorID} dictionary
        print $fh "- Instance: ";
        print $fh "$instance\n";
        print $fh "  SensorID: ";
        print $fh "$occHash{$instance}\n";
    }
    close $fh;
}

# Helper function to put debug statements.
sub printDebug
{
    my $str = shift;
    print "DEBUG: ", $str, "\n" if $debug;
}

# Usage
sub printUsage
{
    print "
    $0 -i [XML filename] -o [Output filename] [OPTIONS]
Options:
    -f = force output file creation even when errors
    -d = debug mode
    -v = verbose mode - for verbose o/p from Targets.pm

PS: mrw::Targets can be found in https://github.com/open-power/serverwiz/
    mrw::Inventory can be found in https://github.com/openbmc/phosphor-mrw-tools/
    \n";
    exit(1);
}
#------------------------------------END OF SUB-----------------------
