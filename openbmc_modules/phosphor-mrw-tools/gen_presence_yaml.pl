#!/usr/bin/env perl

#This script generates YAML that defines the presence detects used
#for FRUs.  Its output is used by code that determines which FRUs
#are present in a system.

use strict;
use warnings;

use Getopt::Long;
use mrw::Inventory;
use mrw::Targets;
use mrw::Util;

my $serverwizFile;
my $outputFile;
GetOptions("i=s" => \$serverwizFile,
           "o=s" => \$outputFile) or printUsage();

if ((not defined $serverwizFile) ||
    (not defined $outputFile))
{
    printUsage();
}

my $targets = Targets->new;
$targets->loadXML($serverwizFile);

my @inventory = Inventory::getInventory($targets);
my %presence;

findTachBasedPresenceDetects(\%presence);

#Future: Find other sorts of presence detects

printYAML(\%presence, $outputFile);

exit 0;


#Finds FRUs and their Presence detects where a tach reading
#is used as the presence detect, such as when a nonzero fan RPM
#tach reading can be used to tell that a particular fan is present.
sub findTachBasedPresenceDetects
{
    my ($presence) = @_;
    my %tachs;

    for my $target (keys %{$targets->getAllTargets()})
    {
        my $connections = $targets->findConnections($target, "TACH");
        next if ($connections eq "");

        for my $tach (sort @{$connections->{CONN}})
        {
            #Because findConnections is recursive, we can hit this same
            #connection multiple times so only use it once.
            next if (exists $tachs{$tach->{SOURCE}}{$tach->{DEST}});
            $tachs{$tach->{SOURCE}}{$tach->{DEST}} = 1;

            my $fru = Util::getEnclosingFru($targets, $tach->{SOURCE});
            my $name = Util::getObmcName(\@inventory, $fru);
            if (not defined $name)
            {
                die "$target was not found in the inventory\n";
            }

            my $sensor = getSensor($tach->{DEST});

            #For now, assuming only fans use tachs
            $$presence{Tach}{$name}{type} = 'Fan';

            #Multi-rotor fans will have > 1 sensors per FRU
            push @{$$presence{Tach}{$name}{sensors}}, $sensor;
        }
    }
}


#Creates the YAML representation of the data
sub printYAML
{
    my ($presence, $outputFile) = @_;
    open (F, ">$outputFile") or die "Could not open $outputFile\n";

    while (my ($method, $FRUs) = each(%{$presence}))
    {
        print F "- $method:\n";
        while (my ($name, $data) = each(%{$FRUs}))
        {
            my ($prettyName) = $name =~ /\b(\w+)$/;

            print F "  - PrettyName: $prettyName\n";
            print F "    Inventory: $name\n";
            print F "    Description:\n"; #purposely leaving blank.
            print F "    Sensors:\n";
            for my $s (@{$data->{sensors}})
            {
                print F "      - $s\n";
            }
        }
    }

    close F;
}


#Find what hwmon will call this unit's reading by looking in
#the child unit-hwmon-feature unit.
sub getSensor
{
    my ($unit) = @_;

    my @hwmons = Util::getChildUnitsWithTargetType($targets,
                                                   "unit-hwmon-feature",
                                                   $unit);
    die "No HWMON children found for $unit\n" unless (scalar @hwmons != 0);

    my $name = $targets->getAttributeField($hwmons[0],
                                           "HWMON_FEATURE",
                                           "DESCRIPTIVE_NAME");
    die "No HWMON name for hwmon unit $hwmons[0]\n" if ($name eq "");

    return $name;
}


sub printUsage
{
    print "$0 -i [XML filename] -o [output YAML filename]\n";
    exit(1);
}
