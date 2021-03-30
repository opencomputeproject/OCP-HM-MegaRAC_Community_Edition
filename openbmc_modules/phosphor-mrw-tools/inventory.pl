#!/usr/bin/env perl

#Creates the OpenBMC inventory from ServerWiz output XML.
#Basically, the inventory includes anything with a FRU name,
#plus some other specific things we always look for since we
#need more than just FRUs.


use strict;
use XML::Simple;
use mrw::Targets;
use Getopt::Long;
use JSON;

my $serverwizFile;
my $outputFile, my $target;
my $fruName, my $type;
my @items, my %item, my %inventory;

#Ensure we never pick these up
my %skipFRUTypes = (OCC => 1);

#We always want the targets with these Types
my %includedTypes = ("CORE" => 1);

#We always want the targets with these MRW Types
my %includedTargetTypes = ("chip-sp-bmc" => 1,
                           "chip-apss-psoc" => 1);

#These are never considered FRUs
my %notFRUTypes = ("CORE" => 1);

GetOptions("x=s" => \$serverwizFile,
           "o=s" => \$outputFile)
or printUsage();

if ((not defined $serverwizFile) || (not defined $outputFile)) {
    printUsage();
}

my $targetObj = Targets->new;
$targetObj->loadXML($serverwizFile);

foreach $target (sort keys %{ $targetObj->getAllTargets() })
{
    $type = $targetObj->getType($target);
    if (exists $skipFRUTypes{$type}) {
        next;
    }

    $fruName = "";

    if (!$targetObj->isBadAttribute($target, "FRU_NAME")) {
        $fruName = $targetObj->getAttribute($target,"FRU_NAME");
    }

    my $targetType = $targetObj->getTargetType($target);

    #We're looking for FRUs, and a few other required parts
    if (($fruName ne "") || (exists $includedTargetTypes{$targetType}) ||
        (exists $includedTypes{$type}))
    {
        $item{name} = $target;
        $item{orig_name} = $target;
        $item{fru_type} = $type;
        $item{target_type} = $targetType;

        if (($fruName ne "") && (not exists $notFRUTypes{$type})) {
            $item{is_fru} = 1;
        } else {
            $item{is_fru} = 0;
        }
        push @items, { %item };
    }

}

#Hardcode the entries that will never be in the MRW
#TODO: openbmc/openbmc#596 Remove when BIOS version is stored elsewhere.
$inventory{'<inventory_root>/system/bios'} =
    {is_fru => 1, fru_type => 'SYSTEM'};

#TODO: openbmc/openbmc#597 Remove when misc FRU data is stored elsewhere.
$inventory{'<inventory_root>/system/misc'} =
    {is_fru => 0, fru_type => 'SYSTEM'};

transform(\@items, \%inventory);

#Encode in JSON and write it out
my $json = JSON->new;
$json->indent(1);
$json->canonical(1);
my $text = $json->encode(\%inventory);

open(FILE, ">$outputFile") or die "Unable to create $outputFile\n";
print FILE $text;
close FILE;

print "Created $outputFile\n";


#Apply OpenBMC naming conventions to the Serverwiz names
sub transform
{
    my $items = shift @_;
    my $inventory = shift @_;

    removeConnectors($items);

    removeProcModule($items);

    renameSegmentWithType("PROC", "cpu", $items);

    renameSegmentWithType("SYS", "system", $items);
    renameType("SYS", "SYSTEM", $items);

    renameSegmentWithType("NODE", "chassis", $items);
    renameType("NODE", "SYSTEM", $items);

    renameSegmentWithTargetType("card-motherboard", "motherboard", $items);
    renameTypeWithTargetType("card-motherboard", "MAIN_PLANAR", $items);

    renameType("MEMBUF", "MEMORY_BUFFER", $items);

    renameType("FSP", "BMC", $items);

    removeCoreParentChiplet($items);

    removeInstNumIfOneInstPresent($items);

    removeHyphensFromInstanceNum($items);

    for my $i (@$items) {
        my $name = "<inventory_root>".$i->{name};
        delete $i->{name};
        delete $i->{orig_name};
        delete $i->{target_type};
        $inventory{$name} = { %$i };
    }
}


#Renames a segment in all target names based on its type
#
#For example:
#    renameSegmentWithType("PROC", "foo", $items)
#  would change
#    /sys-0/node-0/motherboard-0/module-0/cpu/core0
#  to
#    /sys-0/node-0/motherboard-0/module-0/foo/core0
#  assuming /sys-0/.../cpu had type PROC.
sub renameSegmentWithType
{
    my $type = shift @_;
    my $newSegment = shift @_;
    my $items = shift @_;
    my %segmentsToRename;

    for my $item (@$items) {
        my @segments = split('/', $item->{orig_name});
        my $target = "";
        for my $s (@segments) {
            if (length($s) > 0) {
                $target .= "/$s";
                my $curType = "";
                if (!$targetObj->isBadAttribute($target, "TYPE")) {
                    $curType = $targetObj->getType($target);
                }
                if ($curType eq $type) {
                    if (not defined $segmentsToRename{$target}) {
                        my ($oldSegment) = $target =~ /\b(\w+)(-\d+)?$/;
                        $segmentsToRename{$target}{old} = $oldSegment;
                        $segmentsToRename{$target}{new} = $newSegment;
                    }
                }
             }
        }
    }

    for my $s (keys %segmentsToRename) {
        for my $item (@$items) {
            $item->{name} =~
                s/$segmentsToRename{$s}{old}/$segmentsToRename{$s}{new}/;
        }
    }
}


#Renames a segment in all target names based on its target type
#
#For example:
#    renameSegmentWithType("PROC", "foo", $items)
#  would change
#    /sys-0/node-0/motherboard-0/module-0/cpu/core0
#  to
#    /sys-0/node-0/motherboard-0/module-0/foo/core0
#  assuming /sys-0/.../cpu had target type PROC.
sub renameSegmentWithTargetType
{
    my $type = shift @_;
    my $newSegment = shift @_;
    my $items = shift @_;
    my %segmentsToRename;

    for my $item (@$items) {
        my @segments = split('/', $item->{orig_name});
        my $target = "";
        for my $s (@segments) {
            if (length($s) > 0) {
                $target .= "/$s";
                my $curType = $targetObj->getTargetType($target);
                if ($curType eq $type) {
                    if (not defined $segmentsToRename{$target}) {
                        my ($oldSegment) = $target =~ /\b(\w+)(-\d+)?$/;
                        $segmentsToRename{$target}{old} = $oldSegment;
                        $segmentsToRename{$target}{new} = $newSegment;
                    }
                }
             }
        }
    }

    for my $s (keys %segmentsToRename) {
        for my $item (@$items) {
            $item->{name} =~
                s/$segmentsToRename{$s}{old}/$segmentsToRename{$s}{new}/;
        }
    }
}


#Remove the core's parent chiplet, after moving
#the chiplet's instance number to the core.
#Note: Serverwiz always puts the core on a chiplet
sub removeCoreParentChiplet
{
    my $items = shift @_;

    for my $item (@$items) {
        if ($item->{fru_type} eq "CORE") {
            $item->{name} =~ s/\w+-(\d+)\/(\w+)-\d+$/$2-$1/;
        }
    }
}


#Remove path segments that are connectors
sub removeConnectors
{
    my $items = shift @_;
    my %connectors;
    my $item;

    for $item (@$items) {
        my @segments = split('/', $item->{name});
        my $target = "";
        for my $s (@segments) {
            if (length($s) > 0) {
                $target .= "/$s";
                my $class = $targetObj->getAttribute($target, "CLASS");
                if ($class eq "CONNECTOR") {
                    if (not exists $connectors{$target}) {
                        $connectors{$target} = 1;
                    }
                }
            }
        }
    }

    #remove the connector segments out of the path
    #Reverse sort so we start with connectors further out
    for my $connector (sort {$b cmp $a} keys %connectors) {
        for $item (@$items) {
            if ($item->{name} =~ /$connector\b/) {
                my ($inst) = $connector =~ /-(\d+)$/;
                my ($card) = $item->{name};
                $card =~ s/^$connector\///;

                #add the connector instance to the child card
                $card =~ s/^(\w+)-\d+/$1-$inst/;

                #remove the connector segment from the path
                my $base = $connector;
                $base =~ s/\w+-\d+$//;
                $item->{name} = $base . $card;
            }
        }
    }
}


#Remove the processor module card from the path name.
#Note: Serverwiz always outputs proc_socket-X/module-Y/proc.
#      where proc_socket, module, and proc can be any name
#      We already transformed it to module-X/proc.
#      Our use requires proc-X.
#      Note: No multichip modules in plan for OpenPower systems.
sub removeProcModule
{
    my $items = shift @_;
    my $procName = "";

    #Find the name of the processor used in this model
    for my $item (@$items) {
        if ($item->{fru_type} eq "PROC") {
            ($procName) = $item->{name} =~ /\b(\w+)$/;
            last;
        }
    }

    #Now remove it from every instance that it's a part of
    if ($procName eq "") {
        print "Could not find the name of the processor in this system\n";
    } else {
        for my $item (@$items) {
            $item->{name} =~ s/\w+-(\d+)\/$procName/$procName-$1/;
        }
    }
}


sub renameType
{
    my $old = shift @_;
    my $new = shift @_;
    my $items = shift @_;

    for my $item (@$items) {
        $item->{fru_type} =~ s/$old/$new/;
    }
}


sub renameTypeWithTargetType
{
    my $targetType = shift@_;
    my $newType = shift @_;
    my $items = shift @_;

    for my $item (@$items) {
        if ($item->{target_type} eq $targetType) {
            $item->{fru_type} = $newType;
        }
    }
}


sub removeHyphensFromInstanceNum
{
    my $items = shift @_;

    for my $item (@$items) {
        $item->{name} =~ s/-(\d+)\b/$1/g;
    }
}


sub renameSegment
{
    my $old = shift @_;
    my $new = shift @_;
    my $items = shift @_;

    for my $item (@$items) {
      $item->{name} =~ s/\b$old\b/$new/;
    }
}


sub removeInstNumIfOneInstPresent
{
    my $items = shift @_;
    my %instanceHash;
    my $segment, my $item;

    for $item (@$items) {
        my @segments = split('/', $item->{name});

        for $segment (@segments) {
            my ($s, $inst) = $segment =~ /(\w+)-(\d+)/;
            if (defined $s) {
                if (not exists $instanceHash{$s}) {
                    $instanceHash{$s}{inst} = $inst;
                }
                else {
                    if ($instanceHash{$s}{inst} ne $inst) {
                        $instanceHash{$s}{keep} = 1;
                    }
                }
            }
        }
    }

    for my $segment (keys %instanceHash) {

        if (not exists $instanceHash{$segment}{keep}) {
            for $item (@$items) {
               $item->{name} =~ s/$segment-\d+/$segment/;
            }
        }
    }
}


sub printUsage
{
    print "inventory.pl -x [XML filename] -o [output filename]\n";
    exit(1);
}
