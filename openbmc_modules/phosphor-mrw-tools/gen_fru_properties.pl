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
my $skipBrokenMrw = 0;


GetOptions(
"m=s" => \$mrwFile,
"c=s" => \$configFile,
"o=s" => \$outFile,
"skip-broken-mrw" => \$skipBrokenMrw
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


# Parse config YAML
my $targetItems = LoadFile($configFile);


# Targets we're interested in, from the config YAML
my @targetNames = keys %{$targetItems};
my %targetHash;
@targetHash{@targetNames} = ();


# Target Type : Target inventory path
my %defaultPaths = (
    "ETHERNET",
     Util::getObmcName(
         \@inventory,
         Util::getBMCTarget($targets))."/ethernet"
);


open(my $fh, '>', $outFile) or die "Could not open file '$outFile' $!";
# Retrieve OBMC path of targets we're interested in
for my $item (@inventory) {
    my $targetType = "";
    my $path = "";

    if (!$targets->isBadAttribute($item->{TARGET}, "TYPE")) {
        $targetType = $targets->getAttribute($item->{TARGET}, "TYPE");
    }
    next if (not exists $targetItems->{$targetType});

    writeOutput($targetType,
                $item,
                $targetItems);
    delete($targetHash{$targetType});
}
writeRemaining($targetItems);
close $fh;


sub writeRemaining
{
    my ($yamlDict) = @_;
    for my $type (keys %targetHash)
    {
        if($skipBrokenMrw and !exists $defaultPaths{$type})
        {
            next;
        }
        print $fh $defaultPaths{$type}.":";
        print $fh "\n";
        while (my ($interface,$propertyMap) = each %{$yamlDict->{$type}})
        {
            print $fh "    ".$interface.":";
            print $fh "\n";
            while (my ($property,$value) = each %{$propertyMap})
            {
                $value = "'".$value."'";
                print $fh "        ".$property.": ".$value;
                print $fh "\n";
            }
        }
    }
}


sub writeOutput
{
    my ($type, $item, $yamlDict) = @_;
    print $fh $item->{OBMC_NAME}.":";
    print $fh "\n";
    while (my ($interface,$propertyMap) = each %{$yamlDict->{$type}})
    {
        print $fh "    ".$interface.":";
        print $fh "\n";
        while (my ($property,$value) = each %{$propertyMap})
        {
            $value = getValue($item, $property, $value);
            print $fh "        ".$property.": ".$value;
            print $fh "\n";
        }
    }
}


sub getValue
{
    my ($item, $property, $value) = @_;
    $value = "'".$value."'";

    if ($property eq "FieldReplaceable")
    {
        $value = "'false'";
        if (!$targets->isBadAttribute($item->{TARGET}, "RU_TYPE"))
        {
            my $ruType = $targets->getAttribute($item->{TARGET}, "RU_TYPE");
            if (($ruType eq "FRU") || ($ruType eq "CRU"))
            {
                $value = "'true'";
            }
        }
    }

    return $value;
}


sub printUsage
{
    print "
    $0 -m [MRW file] -c [Config yaml] -o [Output filename] [OPTIONS]
Options:
    --skip-broken-mrw = Skip broken MRW targets
    \n";
    exit(1);
}
