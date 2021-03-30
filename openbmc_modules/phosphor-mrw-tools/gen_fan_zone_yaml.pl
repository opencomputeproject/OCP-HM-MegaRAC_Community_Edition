#!/usr/bin/env perl

#This script generates fan definitions from the MRW
#and outputs them in a YAML file.

use strict;
use warnings;

use Getopt::Long;
use mrw::Inventory;
use mrw::Targets;
use mrw::Util;
use Scalar::Util qw(looks_like_number);

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

my %fans = findFans();

printFanYAML(\%fans, $outputFile);


#This function returns a hash representing the fans in the system.
#The hash looks like:
#  $fans{<name>}{<zone>}
#  $fans{<name>}{<profile>}
#  @fans{<name>}{sensors}
#
#  Where:
#    <name> = inventory name
#    <zone> = cooling zone number
#    <profile> = cooling zone profile, such as air, water, or all
#    <sensors> = an array of the hwmon sensors for the fan's tachs
sub findFans
{
    my %tachs;
    my %fans;

    #Find fans by looking at the TACH connections.  We could also find
    #parts of type FAN, but we need the tach connection anyway to
    #lookup the sensors on the other end...
    for my $target (keys %{$targets->getAllTargets()})
    {
        my $connections = $targets->findConnections($target, "TACH");
        next if ($connections eq "");

        for my $tach (sort @{$connections->{CONN}})
        {
            #Because findConnections is recursive, we can hit this same
            #connection multiple times, so only use it once.
            next if (exists $tachs{$tach->{SOURCE}}{$tach->{DEST}});
            $tachs{$tach->{SOURCE}}{$tach->{DEST}} = 1;

            #Note: SOURCE = tach unit on fan, DEST = tach unit on fan ctlr

            my $fru = Util::getEnclosingFru($targets, $tach->{SOURCE});
            my $name = Util::getObmcName(\@inventory, $fru);

            my $sensor = getSensor($tach->{DEST});
            push @{$fans{$name}{sensors}}, $sensor;

            #Get the cooling zone and profile from the fan controller part
            my $part = $targets->getTargetParent($tach->{SOURCE});
            my $zone = $targets->getAttribute($part, "COOLING_ZONE");
            if (!looks_like_number($zone))
            {
                die "Cooling zone '$zone' on $part is not a number\n";
            }

            #If the profile isn't set, just set it to be 'all'.
            my $profile = "";
            if (!$targets->isBadAttribute($part, "COOLING_ZONE_PROFILE"))
            {
                $profile = $targets->getAttribute($part,
                                                  "COOLING_ZONE_PROFILE");
            }

            if ($profile eq "")
            {
                $profile = "all";
            }

            $fans{$name}{profile} = lc $profile;
            $fans{$name}{zone} = $zone;
        }
    }

    return %fans;
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


#Creates the YAML representation of the data
sub printFanYAML
{
    my ($fans, $file) = @_;

    open (F, ">$file") or die "Could not open $file\n";

    print F "fans:\n";

    while (my ($fan, $data) = each(%{$fans}))
    {
        print F "  - inventory: $fan\n";
        print F "    cooling_zone: $data->{zone}\n";
        print F "    cooling_profile: $data->{profile}\n";
        print F "    sensors:\n";
        for my $s (@{$data->{sensors}})
        {
            print F "      - $s\n";
        }
    }

    close F;
}


sub printUsage
{
    print "$0 -i [XML filename] -o [output YAML filename]\n";
    exit(1);
}
