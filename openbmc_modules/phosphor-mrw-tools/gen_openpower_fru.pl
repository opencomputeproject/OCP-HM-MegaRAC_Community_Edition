#! /usr/bin/perl
use strict;
use warnings;

use mrw::Targets;
use mrw::Inventory;
use mrw::Util;
use Getopt::Long;
use YAML::Tiny qw(LoadFile);

my $mrwFile = "";
my $outFile = "";
my $configFile = "";

GetOptions(
"m=s" => \$mrwFile,
"c=s" => \$configFile,
"o=s" => \$outFile,
)
or printUsage();

if (($mrwFile eq "") or ($configFile eq "") or ($outFile eq ""))
{
    printUsage();
}

# Load system MRW
my $targets = Targets->new;
$targets->loadXML($mrwFile);
my @inventory = Inventory::getInventory($targets);

# Target Type : Target inventory path
my %defaultPaths = (
    "ETHERNET",
     Util::getObmcName(
         \@inventory,
         Util::getBMCTarget($targets))."/ethernet"
);

# Parse config YAML
my $targetItems = LoadFile($configFile);

# Targets we're interested in, from the config YAML
my @targetNames = keys %{$targetItems};
my %targetHash;
@targetHash{@targetNames} = ();
my @targetTypes;
my @paths;

# Retrieve OBMC path of targets we're interested in
for my $item (@inventory) {
    my $targetType = "";
    my $path = "";

    if (!$targets->isBadAttribute($item->{TARGET}, "TYPE")) {
        $targetType = $targets->getAttribute($item->{TARGET}, "TYPE");
    }

    my @types = grep(/$targetType$/, @targetTypes);

    next if (length($targetType) == 0 ||
             ((not exists $targetHash{$targetType}) && (scalar @types == 0)));

    push @targetTypes, $targetType;
    push @paths, $item->{OBMC_NAME};
    delete($targetHash{$targetType});
}

for my $type (keys %targetHash)
{
    if(defined $defaultPaths{$type})
    {
        # One or more targets wasn't present in the inventory
        push @targetTypes, $type;
        push @paths, $defaultPaths{$type};
    }
}

open(my $fh, '>', $outFile) or die "Could not open file '$outFile' $!";
print $fh "FRUS=".join ',',@targetTypes;
print $fh "\n";
print $fh "PATHS=".join ',',@paths;
close $fh;

sub printUsage
{
    print "
    $0 -m [MRW file] -c [Config yaml] -o [Output filename]\n";
    exit(1);
}
