#! /usr/bin/perl

# This script is used for generating callout lists from the MRW for devices
# that can be accessed from the BMC.  The callouts have a location code, the
# target name (for debug), a priority, and in some cases a MRU.  The script
# supports I2C, FSI, SPI, FSI-I2C, and FSI-SPI devices.  The output is a JSON
# file organized into sections for each callout type, with keys based on the
# type.  I2c uses a bus and address, FSI uses a link, and SPI uses a bus
# number. If FSI is combined with I2C or SPI, then the link plus the I2C/SPI
# keys is used.  Multi-hop FSI links are indicated by a dash in between the
# links, eg "0-1".
#
# An example section is:
# "FSI":
# {
#   "5":
#   {
#      "Callouts":[
#        {
#           "Priority":"H"
#           "LocationCode": "P1-C50",
#           "MRU":"/sys-0/node-0/motherboard/proc_socket-0/module-0/power9-0",
#           "Name":"/sys-0/node-0/motherboard/cpu0"
#        },
#        {
#           "Priority":"H",
#           "LocationCode": "P1-C42",
#           "MRU":"/sys-0/node-0/motherboard/ebmc-card/BMC-0",
#           "Name":"/sys-0/node-0/motherboard/ebmc-card"
#        },
#        {
#           "Priority":"L",
#           "LocationCode": "P1",
#           "Name":"/sys-0/node-0/motherboard"
#        }
#     ],
#     "Dest":"/sys-0/node-0/motherboard-0/proc_socket-0/module-0/power9-0",
#     "Source":"/sys-0/node-0/motherboard-0/ebmc-card-connector-0/card-0/bmc-0"
#   }
# }
# The Name, Dest and Source entries are MRW targets and are just for debug.
#
# The optional --segments argument will output a JSON file of all the bus
# segments in the system, which is what the callouts are made from.

use strict;
use warnings;

# Callout object
# Base class for other callouts.
# There is an object per device, so it can contain multiple
# FRU callouts in the calloutList attribute.
package Callout;
sub new
{
    my $class = shift;
    my $self = {
        type => shift,
        sourceChip => shift,
        destChip => shift,
        calloutList => shift,
    };

    return bless $self, $class;
}

sub sourceChip
{
    my $self = shift;
    return $self->{sourceChip};
}

sub destChip
{
    my $self = shift;
    return $self->{destChip};
}

sub type
{
    my $self = shift;
    return $self->{type};
}

sub calloutList
{
    my $self = shift;
    return $self->{calloutList};
}

# I2CCallout object for I2C callouts
package I2CCallout;
our @ISA = qw(Callout);
sub new
{
    my ($class) = @_;
    # source, dest, calloutList
    my $self = $class->SUPER::new("I2C", $_[1], $_[2], $_[3]);
    $self->{i2cBus} = $_[4];
    $self->{i2cAddr} = $_[5];
    return bless $self, $class;
}

sub i2cBus
{
    my $self = shift;
    return $self->{i2cBus};
}

sub i2cAddress
{
    my $self = shift;
    return $self->{i2cAddr};
}

# FSICallout object for FSI callouts
package FSICallout;
our @ISA = qw(Callout);
sub new
{
    my ($class) = @_;
    my $self = $class->SUPER::new("FSI", $_[1], $_[2], $_[3]);
    $self->{FSILink} = $_[4];
    bless $self, $class;
    return $self;
}

sub fsiLink
{
    my $self = shift;
    return $self->{FSILink};
}

# SPICallout object for SPI callouts
package SPICallout;
our @ISA = qw(Callout);
sub new
{
    my ($class) = @_;
    my $self = $class->SUPER::new("SPI", $_[1], $_[2], $_[3]);
    $self->{SPIBus} = $_[4];
    bless $self, $class;
    return $self;
}

sub spiBus
{
    my $self = shift;
    return $self->{SPIBus};
}

# FSII2CCallout object for FSI-I2C callouts
# Ideally this would be derived from FSICallout, but I can't
# get it to work.
package FSII2CCallout;

our @ISA = qw(Callout);
sub new
{
    my ($class) = @_;
    # source, dest, calloutList
    my $self = $class->SUPER::new("FSI-I2C", $_[1], $_[2], $_[3]);
    $self->{FSILink} = $_[4];
    $self->{i2cBus} = $_[5];
    $self->{i2cAddr} = $_[6];
    return bless $self, $class;
}

sub fsiLink
{
    my $self = shift;
    return $self->{FSILink};
}

sub i2cBus
{
    my $self = shift;
    return $self->{i2cBus};
}

sub i2cAddress
{
    my $self = shift;
    return $self->{i2cAddr};
}

#FSISPICallout object for FSI-SPI callouts
package FSISPICallout;

our @ISA = qw(Callout);
sub new
{
    my ($class) = @_;
    # source, dest, calloutList
    my $self = $class->SUPER::new("FSI-SPI", $_[1], $_[2], $_[3]);
    $self->{FSILink} = $_[4];
    $self->{SPIBus} = $_[5];
    return bless $self, $class;
}

sub fsiLink
{
    my $self = shift;
    return $self->{FSILink};
}

sub spiBus
{
    my $self = shift;
    return $self->{SPIBus};
}

package main;

use mrw::Targets;
use mrw::Util;
use Getopt::Long;
use File::Basename;
use JSON;

my $mrwFile = "";
my $outFile = "";
my $printSegments = 0;

# Not supporting priorites A, B, or C until necessary
my %priorities = (H => 3, M => 2, L => 1);

# Segment bus types
my %busTypes = ( I2C => 1, FSIM => 1, FSICM => 1, SPI => 1 );

GetOptions(
    "m=s" => \$mrwFile,
    "o=s" => \$outFile,
    "segments" => \$printSegments
)
    or printUsage();

if (($mrwFile eq "") or ($outFile eq ""))
{
    printUsage();
}

# Load system MRW
my $targets = Targets->new;
$targets->loadXML($mrwFile);

# Find all single segment buses that we care about
my %allSegments = getPathSegments();

my @callouts;

# Build the single and multi segment callouts
buildCallouts(\%allSegments, \@callouts);

printJSON(\@callouts);

# Write the segments to a JSON file
if ($printSegments)
{
    my $outDir = dirname($outFile);
    my $segmentsFile = "$outDir/segments.json";

    open(my $fh, '>', $segmentsFile) or
        die "Could not open file '$segmentsFile' $!";

    my $json = JSON->new;
    $json->indent(1);
    $json->canonical(1);
    my $text = $json->encode(\%allSegments);
    print $fh $text;
    close $fh;
}

# Returns a hash of all the FSI, I2C, and SPI segments in the MRW
sub getPathSegments
{
    my %segments;
    foreach my $target (sort keys %{$targets->getAllTargets()})
    {
        my $numConnections = $targets->getNumConnections($target);

        if ($numConnections == 0)
        {
            next;
        }

        for (my $connIndex=0;$connIndex<$numConnections;$connIndex++)
        {
            my $connBusObj = $targets->getConnectionBus($target, $connIndex);
            my $busType = $connBusObj->{bus_type};

            # We only care about certain bus types
            if (not exists $busTypes{$busType})
            {
                next;
            }

            my $dest = $targets->getConnectionDestination($target, $connIndex);

            my %segment;
            $segment{BusType} = $busType;
            $segment{SourceUnit} = $target;
            $segment{SourceChip} = getParentByClass($target, "CHIP");
            if ($segment{SourceChip} eq "")
            {
                die "Warning: Could not get parent chip for source $target\n";
            }

            $segment{DestUnit} = $dest;
            $segment{DestChip} = getParentByClass($dest, "CHIP");

            # If the unit's direct parent is a connector that's OK too.
            if ($segment{DestChip} eq "")
            {
                my $parent = $targets->getTargetParent($dest);
                if ($targets->getAttribute($parent, "CLASS") eq "CONNECTOR")
                {
                    $segment{DestChip} = $parent;
                }
            }

            if ($segment{DestChip} eq "")
            {
                die "Warning: Could not get parent chip for dest $dest\n";
            }

            my $fruPath = $targets->getBusAttribute(
                $target, $connIndex, "FRU_PATH");

            if (defined $fruPath)
            {
                $segment{FRUPath} = $fruPath;
                my @callouts = getFRUPathCallouts($fruPath);
                $segment{Callouts} = \@callouts;
            }
            else
            {
                $segment{FRUPath} = "";
                my @empty;
                $segment{Callouts} = \@empty;
            }

            if ($busType eq "I2C")
            {
                $segment{I2CBus} = $targets->getAttribute($target, "I2C_PORT");
                $segment{I2CAddress} =
                    hex($targets->getAttribute($dest, "I2C_ADDRESS"));

                $segment{I2CBus} = $segment{I2CBus};

                # Convert to the 7 bit address that linux uses
                $segment{I2CAddress} =
                    Util::adjustI2CAddress($segment{I2CAddress});
            }
            elsif ($busType eq "FSIM")
            {
                $segment{FSILink} =
                    hex($targets->getAttribute($target, "FSI_LINK"));
            }
            elsif ($busType eq "SPI")
            {
                $segment{SPIBus} = $targets->getAttribute($target, "SPI_PORT");

                # Seems to be in HEX sometimes
                if ($segment{SPIBus} =~ /^0x/i)
                {
                    $segment{SPIBus} = hex($segment{SPIBus});
                }
            }

            push @{$segments{$busType}}, { %segment };
        }
    }

    return %segments;
}

#Breaks the FRU_PATH atttribute up into its component callouts.
#It looks like:  "H:<some target>,L:<some other target>(<MRU>)"
#Where H/L are the priorities and can be H/M/L.
#The MRU that is in parentheses is optional and is a chip name on that
#FRU target.
sub getFRUPathCallouts
{
    my @callouts;
    my $fruPath = shift;

    my @entries = split(',', $fruPath);

    for my $entry (@entries)
    {
        my %callout;
        my ($priority, $path) = split(':', $entry);

        # pull the MRU out of the parentheses at the end and then
        # remove the parentheses.
        if ($path =~ /\(.+\)$/)
        {
            ($callout{MRU}) = $path =~ /\((.+)\)/;

            $path =~ s/\(.+\)$//;
        }

        # check if the target we read out is valid by
        # checking for a required attribute
        if ($targets->isBadAttribute($path, "CLASS"))
        {
            die "FRU Path $path not a valid target\n";
        }

        $callout{Priority} = $priority;
        if (not exists $priorities{$priority})
        {
            die "Invalid priority: '$priority' on callout $path\n";
        }

        $callout{Name} = $path;

        push @callouts, \%callout;
    }

    return @callouts;
}

# Returns an ancestor target based on its class
sub getParentByClass
{
    my ($target, $class) = @_;
    my $parent = $targets->getTargetParent($target);

    while (defined $parent)
    {
        if (!$targets->isBadAttribute($parent, "CLASS"))
        {
            if ($class eq $targets->getAttribute($parent, "CLASS"))
            {
                return $parent;
            }
        }
        $parent = $targets->getTargetParent($parent);
    }

    return "";
}

# Build the callout objects
sub buildCallouts
{
    my ($segments, $callouts) = @_;

    # Callouts for 1 segment connections directly off of the BMC.
    buildBMCSingleSegmentCallouts($segments, $callouts);

    # Callouts more than 1 segment away
    buildMultiSegmentCallouts($segments, $callouts);
}

# Build the callout objects for devices 1 segment away.
sub buildBMCSingleSegmentCallouts
{
    my ($segments, $callouts) = @_;

    for my $busType (keys %$segments)
    {
        for my $segment (@{$$segments{$busType}})
        {
            my $chipType = $targets->getType($segment->{SourceChip});
            if ($chipType eq "BMC")
            {
                my $callout = buildSingleSegmentCallout($segment);

                if (defined $callout)
                {
                    push @{$callouts}, $callout;
                }
            }
        }
    }
}

# Build the callout object based on the callout type using the
# callout list from the single segment.
sub buildSingleSegmentCallout
{
    my ($segment, $callouts) = @_;

    if ($segment->{BusType} eq "I2C")
    {
        return createI2CCallout($segment, $callouts);
    }
    elsif ($segment->{BusType} eq "FSIM")
    {
        return createFSICallout($segment, $callouts);
    }
    elsif ($segment->{BusType} eq "SPI")
    {
        return createSPICallout($segment, $callouts);
    }

    return undef;
}

# Build the callout objects for devices more than 1 segment away from
# the BMC.  All but the last segment will always be FSI.  The FRU
# callouts accumulate as segments are added.
sub buildMultiSegmentCallouts
{
    my ($segments, $callouts) = @_;
    my $hops = 0;
    my $found = 1;

    # Connect FSI link callouts to other FSI segments to make new callouts, and
    # connect all FSI link callouts up with the I2C/SPI segments to make even
    # more new callouts.  Note: Deal with I2C muxes, if they are ever modeled,
    # when there are some.

    # Each time through the loop, go out another FSI hop.
    # Stop when no more new hops are found.
    while ($found)
    {
        my @newCallouts;
        $found = 0;

        for my $callout (@$callouts)
        {
            if ($callout->type() ne "FSI")
            {
                next;
            }

            # link numbers are separated by '-'s in the link field,
            # so 0-5 = 1 hop
            my @numHops = $callout->fsiLink() =~ /(-)/g;

            # only deal with callout objects that contain $hops hops.
            if ($hops != scalar @numHops)
            {
                next;
            }

            for my $segmentType (keys %$segments)
            {
                for my $segment (@{$$segments{$segmentType}})
                {
                    # If the destination on this callout is the same
                    # as the source of the segment, then make a new
                    # callout that spans both.
                    if ($callout->destChip() eq $segment->{SourceChip})
                    {
                        # First build the new single segment callout
                        my $segmentCallout =
                            buildSingleSegmentCallout($segment);

                        # Now merge both callouts into one.
                        if (defined $segmentCallout)
                        {
                            my $newCallout =
                                mergeCallouts($callout, $segmentCallout);

                            push @newCallouts, $newCallout;
                            $found = 1;
                        }
                    }
                }
            }
        }

        if ($found)
        {
            push @{$callouts}, @newCallouts;
        }

        $hops = $hops + 1;
    }
}

# Merge 2 callout objects into 1 that contains all of their FRU callouts.
sub mergeCallouts
{
    my ($firstCallout, $secondCallout) = @_;

    # This callout list will be merged/sorted later.
    # Endpoint callouts are added first, so they will be higher
    # in the callout list (priority permitting).
    my @calloutList;
    push @calloutList, @{$secondCallout->calloutList()};
    push @calloutList, @{$firstCallout->calloutList()};

    # FSI
    if (($firstCallout->type() eq  "FSI") && ($secondCallout->type() eq "FSI"))
    {
        # combine the FSI links with a -
        my $FSILink = $firstCallout->fsiLink() . "-" .
            $secondCallout->fsiLink();

        my $fsiCallout =  new FSICallout($firstCallout->sourceChip(),
            $secondCallout->destChip(), \@calloutList, $FSILink);

        return $fsiCallout;
    }
    # FSI-I2C
    elsif (($firstCallout->type() eq  "FSI") &&
        ($secondCallout->type() eq "I2C"))
    {
        my $i2cCallout = new FSII2CCallout($firstCallout->sourceChip(),
            $secondCallout->destChip(), \@calloutList,
            $firstCallout->fsiLink(),  $secondCallout->i2cBus(),
            $secondCallout->i2cAddress());

        return $i2cCallout;
    }
    # FSI-SPI
    elsif (($firstCallout->type() eq  "FSI") &&
        ($secondCallout->type() eq "SPI"))
    {
        my $spiCallout = new FSISPICallout($firstCallout->sourceChip(),
            $secondCallout->destChip(), \@calloutList,
            $firstCallout->fsiLink(), $secondCallout->spiBus());

        return $spiCallout;
    }

    die "Unrecognized callouts to merge: " . $firstCallout->type() .
    " " . $secondCallout->type() . "\n";
}

# Create an I2CCallout object
sub createI2CCallout
{
    my $segment = shift;
    my $bus = $segment->{I2CBus};

    # Convert MRW BMC I2C numbering to the linux one for the BMC
    if ($targets->getAttribute($segment->{SourceChip}, "TYPE") eq "BMC")
    {
        $bus = Util::adjustI2CPort($segment->{I2CBus});

        if ($bus < 0)
        {
            die "After adjusting BMC I2C bus $segment->{I2CBus}, " .
                "got a negative number\n";
        }
    }

    my $i2cCallout = new I2CCallout($segment->{SourceChip},
        $segment->{DestChip}, $segment->{Callouts}, $bus,
        $segment->{I2CAddress});

    return $i2cCallout;
}

# Create an FSICallout object
sub createFSICallout
{
    my $segment = shift;

    my $fsiCallout = new FSICallout($segment->{SourceChip},
        $segment->{DestChip}, $segment->{Callouts},
        $segment->{FSILink}, $segment);

    return $fsiCallout;
}

# Create a SPICallout object
sub createSPICallout
{
    my $segment = shift;

    my $spiCallout = new SPICallout($segment->{SourceChip},
        $segment->{DestChip}, $segment->{Callouts},
        $segment->{SPIBus});

    return $spiCallout;
}

# Convert all of the *Callout objects to JSON and write them to a file.
# It will convert the callout target names to location codes and also sort
# the callout list before doing so.
sub printJSON
{
    my $callouts = shift;
    my %output;

    for my $callout (@$callouts)
    {
        my %c;
        $c{Source} = $callout->sourceChip();
        $c{Dest} = $callout->destChip();

        for my $fruCallout (@{$callout->calloutList()})
        {
            my %entry;

            if (length($fruCallout->{Name}) == 0)
            {
                die "Could not get target name for a callout on path:\n" .
                 "    (" . $callout->sourceChip() . ") ->\n" .
                 "    (" . $callout->destChip() . ")\n";
            }

            $entry{Name} = $fruCallout->{Name};

            $entry{LocationCode} =
                Util::getLocationCode($targets, $fruCallout->{Name});

            # MRUs - for now just use the path + MRU name
            if (exists $fruCallout->{MRU})
            {
                $entry{MRU} = $entry{Name} . "/" . $fruCallout->{MRU};
            }

            $entry{Priority} = validatePriority($fruCallout->{Priority});

            push @{$c{Callouts}}, \%entry;
        }


        # Remove duplicate callouts and sort
        @{$c{Callouts}} = sortCallouts(@{$c{Callouts}});

        # Setup the entry based on the callout type
        if ($callout->type() eq "I2C")
        {
            # The address key is in decimal, but save the hex value
            # for easier debug.
            $c{HexAddress} = $callout->i2cAddress();
            my $decimal = hex($callout->i2cAddress());

            $output{"I2C"}{$callout->i2cBus()}{$decimal} = \%c;
        }
        elsif ($callout->type() eq "SPI")
        {
            $output{"SPI"}{$callout->spiBus()} = \%c;
        }
        elsif ($callout->type() eq "FSI")
        {
            $output{"FSI"}{$callout->fsiLink()} = \%c;
        }
        elsif ($callout->type() eq "FSI-I2C")
        {
            $c{HexAddress} = $callout->i2cAddress();
            my $decimal = hex($callout->i2cAddress());

            $output{"FSI-I2C"}{$callout->fsiLink()}
                {$callout->i2cBus()}{$decimal} = \%c;
        }
        elsif ($callout->type() eq "FSI-SPI")
        {
            $output{"FSI-SPI"}{$callout->fsiLink()}{$callout->spiBus()} = \%c;
        }
    }

    open(my $fh, '>', $outFile) or die "Could not open file '$outFile' $!";
    my $json = JSON->new->utf8;
    $json->indent(1);
    $json->canonical(1);
    my $text = $json->encode(\%output);
    print $fh $text;
    close $fh;
}

# This will remove duplicate callouts from the input callout list, keeping
# the highest priority value and MRU, and then also sort by priority.
#
# There could be duplicates when multiple single segment callouts are
# combined into 1.
sub sortCallouts
{
    my @callouts = @_;

    # This will undef the duplicates, and then remove them at the end,
    for (my $i = 0; $i < (scalar @callouts) - 1; $i++)
    {
        next if not defined $callouts[$i];

        for (my $j = $i + 1; $j < (scalar @callouts); $j++)
        {
            next if not defined $callouts[$j];

            if ($callouts[$i]->{LocationCode} eq $callouts[$j]->{LocationCode})
            {
                # Keep the highest priority value
                $callouts[$i]->{Priority} = getHighestPriority(
                    $callouts[$i]->{Priority}, $callouts[$j]->{Priority});

                # Keep the MRU if present
                if (defined $callouts[$j]->{MRU})
                {
                    $callouts[$i]->{MRU} = $callouts[$j]->{MRU};
                }

                $callouts[$j] = undef;
            }
        }
    }

    # removed the undefined ones
    @callouts = grep {defined ($_)} @callouts;

    # sort from highest to lowest priorities
    @callouts = sort {
        $priorities{$b->{Priority}} <=> $priorities{$a->{Priority}}
    } @callouts;

    return @callouts;
}

# Returns the highest priority value of the two passed in
sub getHighestPriority
{
    my ($p1, $p2) = @_;

    if ($priorities{$p1} > $priorities{$p2})
    {
        return $p1;
    }
    return $p2;
}

# Dies if the input priority isn't valid
sub validatePriority
{
    my $priority = shift;

    if (not exists $priorities{$priority})
    {
        die "Invalid callout priority found: $priority\n";
    }

    return $priority;
}

sub printUsage
{
    print "$0 -m <MRW file> -o <Output filename> [--segments] [-n]\n" .
    "        -m <MRW file> = The MRW XML\n" .
    "        -o <Output filename> = The output JSON\n" .
    "        [--segments] = Optionally create a segments.json file\n" .
    exit(1);
}
