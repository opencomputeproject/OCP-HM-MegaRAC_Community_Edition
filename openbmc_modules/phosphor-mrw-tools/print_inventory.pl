#! /usr/bin/perl

use strict;
use warnings;
use mrw::Targets;
use mrw::Inventory;

my $targetObj;
my $serverwizFile = $ARGV[0];
if ((not defined $serverwizFile) || (! -e $serverwizFile)) {
    die "Usage:  $0 [XML filename]\n";
}

$targetObj = Targets->new;
$targetObj->loadXML($serverwizFile);

my @inventory = Inventory::getInventory($targetObj);

for my $item (@inventory) {
    print "---------------------------------------------------------------\n";
    print "Target:  $item->{TARGET}\n";
    print "Name:    $item->{OBMC_NAME}\n";
}
