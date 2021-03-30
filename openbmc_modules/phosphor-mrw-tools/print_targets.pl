#! /usr/bin/perl

use strict;
use warnings;
use mrw::Targets;

my $targetObj;
my $serverwizFile = $ARGV[0];
if ((not defined $serverwizFile) || (! -e $serverwizFile)) {
    die "Usage:  $0 [XML filename]\n";
}

$targetObj = Targets->new;
$targetObj->loadXML($serverwizFile);

dumpMRW($targetObj);

sub dumpMRW
{
    my ($targetObj) = @_;

    for my $target (sort keys %{$targetObj->getAllTargets()}) {
        print "-----------------------------------------------------------\n";
        print "Target $target\n";
        my $thash = $targetObj->getTarget($target);

        for my $attr (keys %{$thash->{ATTRIBUTES}}) {
            print "\t$attr:  ";

            if (ref($thash->{ATTRIBUTES}->{$attr}->{default}) eq "HASH") {
                print "\n";

                for my $f (sort keys %{$thash->{ATTRIBUTES}->
                        {$attr}->{default}->{field}}) {

                    my $val = $thash->{ATTRIBUTES}->
                        {$attr}->{default}->{field}->{$f}->{value};
                    print "\t\t$f:  $val\n";
                }
            }
            else {
                print $thash->{ATTRIBUTES}->{$attr}->{default} . "\n";
            }
        }
    }
}
