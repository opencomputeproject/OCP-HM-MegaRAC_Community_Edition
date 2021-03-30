#!/usr/bin/perl
use strict;
use warnings;

use mrw::Targets;
use mrw::Inventory;
use mrw::Util;
use Getopt::Long; # For parsing command line arguments
use YAML::Tiny qw(LoadFile);
# Globals
my $serverwizFile  = "";
my $debug           = 0;
my $outputFile     = "";
my $metaDataFile   = "";
my $skipBrokenMrw  = 0;

# Command line argument parsing
GetOptions(
"i=s" => \$serverwizFile,    # string
"m=s" => \$metaDataFile,     # string
"o=s" => \$outputFile,       # string
"skip-broken-mrw" => \$skipBrokenMrw,
"d"   => \$debug,
)
or printUsage();

if (($serverwizFile eq "") or ($outputFile eq "") or ($metaDataFile eq ""))
{
    printUsage();
}

my $targetObj = Targets->new;
$targetObj->loadXML($serverwizFile);

#open the mrw xml and the metaData file for the fru.
#Fetch the FRU id,type,object path from the mrw.
#Get the metadata for that fru from the metadata file.
#Merge the data into the outputfile

open(my $fh, '>', $outputFile) or die "Could not open file '$outputFile' $!";
my $fruTypeConfig = LoadFile($metaDataFile);

my @interestedTypes = keys %{$fruTypeConfig};
my %types;
@types{@interestedTypes} = ();

my %entityInfoDict;

my @allAssoTypes = getAllAssociatedTypes($fruTypeConfig);
my %allAssoTypesHash;
@allAssoTypesHash{@allAssoTypes} = ();

my @inventory = Inventory::getInventory($targetObj);
for my $item (@inventory) {
    my $isFru = 0, my $fruID = 0, my $fruType = "";
    my $entityInstance = 1, my $entityID = 0;

    #Fetch the FRUID.
    if (!$targetObj->isBadAttribute($item->{TARGET}, "FRU_ID")) {
        $fruID = $targetObj->getAttribute($item->{TARGET}, "FRU_ID");
        $isFru = 1;
    }
    #Fetch the FRU Type.
    if (!$targetObj->isBadAttribute($item->{TARGET}, "TYPE")) {
        $fruType = $targetObj->getAttribute($item->{TARGET}, "TYPE");
    }

    #Skip if any one is true
    #1) If not fru
    #2) if the fru type is not there in the config file.
    #3) if the fru type is in associated types.
    #4) if FRU_ID is not defined for the target and we are asked to ignore such
    #   targets.

    next if ($skipBrokenMrw and ($fruID eq ""));

    next if (not $isFru or not exists $types{$fruType} or exists $allAssoTypesHash{$fruType});

    printDebug ("FRUID => $fruID, FRUType => $fruType, ObjectPath => $item->{OBMC_NAME}");

    print $fh $fruID.":";
    print $fh "\n";

    $entityID = $fruTypeConfig->{$fruType}->{'EntityID'};

    if (exists $entityInfoDict{$entityID}) {
        $entityInstance = $entityInfoDict{$entityID} + 1;
    }

    printDebug("entityID => $entityID , entityInstance => $entityInstance");

    $entityInfoDict{$entityID} = $entityInstance;

    writeToFile($fruType,$item->{OBMC_NAME},$fruTypeConfig,$fh);

    #if the key(AssociatedTypes) exists and  it is defined
    #then make the association.

    if (!defined $fruTypeConfig->{$fruType}->{'AssociatedTypes'}) {
        next;
    }

    my $assoTypes = $fruTypeConfig->{$fruType}->{'AssociatedTypes'};
    for my $type (@$assoTypes) {
        my @devices = Util::getDevicePath(\@inventory,$targetObj,$type);
        for my $device (@devices) {
            writeToFile($type,$device,$fruTypeConfig,$fh);
        }

    }

}
close $fh;

#------------------------------------END OF MAIN-----------------------

# Get all the associated types
sub getAllAssociatedTypes
{
   my $fruTypeConfig = $_[0];
   my @assoTypes;
   while (my($key, $value) = each %$fruTypeConfig) {
        #if the key exist and value is also there
        if (defined $value->{'AssociatedTypes'}) {
            my $assoTypes = $value->{'AssociatedTypes'};
            for my $type (@$assoTypes) {
                push(@assoTypes,$type);
            }
        }
   }
   return @assoTypes;
}

#Get the metdata for the incoming frutype from the loaded config file.
#Write the FRU data into the output file

sub writeToFile
{
    my $fruType = $_[0];#fru type
    my $instancePath = $_[1];#instance Path
    my $fruTypeConfig = $_[2];#loaded config file (frutypes)
    my $fh = $_[3];#file Handle
    #walk over all the fru types and match for the incoming type

    print $fh "  ".$instancePath.":";
    print $fh "\n";
    print $fh "    "."entityID: ".$fruTypeConfig->{$fruType}->{'EntityID'};
    print $fh "\n";
    print $fh "    "."entityInstance: ";
    print $fh $entityInfoDict{$fruTypeConfig->{$fruType}->{'EntityID'}};
    print $fh "\n";
    print $fh "    "."interfaces:";
    print $fh "\n";

    my $interfaces = $fruTypeConfig->{$fruType}->{'Interfaces'};

    #Walk over all the interfaces as it needs to be written
    while ( my ($interface,$properties) = each %{$interfaces}) {
        print $fh "      ".$interface.":";
        print $fh "\n";
        #walk over all the properties as it needs to be written
        while ( my ($dbusProperty,$metadata) = each %{$properties}) {
                    #will write property named "Property" first then
                    #other properties.
            print $fh "        ".$dbusProperty.":";
            print $fh "\n";
            for my $key (sort keys %{$metadata}) {
                print $fh "          $key: "."$metadata->{$key}";
                print $fh "\n";
            }
        }
    }
}

# Usage
sub printUsage
{
    print "
    $0 -i [MRW filename] -m [MetaData filename] -o [Output filename] [OPTIONS]
Options:
    -d = debug mode
    --skip-broken-mrw = Skip broken MRW targets
        \n";
    exit(1);
}

# Helper function to put debug statements.
sub printDebug
{
    my $str = shift;
    print "DEBUG: ", $str, "\n" if $debug;
}

