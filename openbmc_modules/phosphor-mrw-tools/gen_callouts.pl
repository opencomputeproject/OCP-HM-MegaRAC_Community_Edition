#! /usr/bin/perl
use strict;
use warnings;


use mrw::Targets;
use mrw::Inventory;
use mrw::Util;
use Getopt::Long;


my $mrwFile = "";
my $outFile = "";


GetOptions(
"m=s" => \$mrwFile,
"o=s" => \$outFile,
)
or printUsage();


if (($mrwFile eq "") or ($outFile eq ""))
{
    printUsage();
}


# Load system MRW
my $targets = Targets->new;
$targets->loadXML($mrwFile);


# Load inventory
my @inventory = Inventory::getInventory($targets);


# paths
my $i2cPath = "/sys/devices/platform/ahb/ahb:apb/ahb:apb:bus\@1e78a000/1e78a100.i2c-bus/i2c-<port>/<port>-00<address>";
my $fsiMasterPath = "/sys/devices/platform/gpio-fsi/fsi0/slave\@00:00/raw";
my $fsiSlavePath = "/sys/devices/platform/gpio-fsi/fsi0/slave\@00:00/00:00:00:0a/fsi1/slave\@<link>:00/raw";


open(my $fh, '>', $outFile) or die "Could not open file '$outFile' $!";

genI2CCallouts();
genProcFSICallouts();

close $fh;


sub genI2CCallouts
{
    my $bmc = Util::getBMCTarget($targets);
    my $connections = $targets->findConnections($bmc, "I2C");
    # hash of arrays - {I2C master port : list of connected slave Targets}
    my %masters;

    for my $i2c (@{$connections->{CONN}})
    {
        my $master = $i2c->{SOURCE};
        my $port = $targets->getAttribute($master,"I2C_PORT");
        $port = Util::adjustI2CPort($port);
        my $slave = $i2c->{DEST};
        push(@{$masters{$port}}, $slave);
    }

    for my $m (keys %masters)
    {
        for my $s(@{$masters{$m}})
        {
            my $addr = $targets->getAttribute($s,"I2C_ADDRESS");
            $addr = Util::adjustI2CAddress(hex($addr));
            $addr = substr $addr, 2; # strip 0x
            my $path = $i2cPath;
            $path =~ s/<port>/$m/g;
            $path =~ s/<address>/$addr/g;
            print $fh $path.": ";
            my $fru = Util::getEnclosingFru($targets, $s);
            print $fh Util::getObmcName(\@inventory, $fru)."\n";
        }
    }
}


sub genProcFSICallouts
{
    my @procs;
    for my $target (keys %{$targets->getAllTargets()})
    {
        if ($targets->getType($target) eq "PROC")
        {
            push @procs, $target;
        }
    }

    for my $proc (@procs)
    {
        my $connections = $targets->findConnections($proc, "FSIM");
        if ("" ne $connections)
        {
            # This is a master processor
            my $path = $fsiMasterPath; # revisit on a multinode system
            my $fru = Util::getEnclosingFru($targets, $proc);
            print $fh $path.": ".Util::getObmcName(\@inventory, $fru);
            for my $fsi (@{$connections->{CONN}})
            {
                my $master = $fsi->{SOURCE};
                my $slave = $fsi->{DEST};
                my $link = $targets->getAttribute($master, "FSI_LINK");
                $link = substr $link, 2; # strip 0x
                my $fru = Util::getEnclosingFru($targets, $slave);
                $path = $fsiSlavePath;
                $path =~ s/<link>/$link/g;
                print $fh "\n".$path.": ".Util::getObmcName(\@inventory, $fru);
            }
        }
    }
}


sub printUsage
{
    print "
    $0 -m [MRW file] -o [Output filename]\n";
    exit(1);
}
