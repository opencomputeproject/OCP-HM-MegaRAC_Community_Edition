package Inventory;

use strict;
use warnings;

#Target types to always include in the inventory if present
my %TYPES = (SYS => 1, NODE => 1, PROC => 1,
             BMC => 1, GPU => 1, CORE => 1, OCC => 1, TPM => 1);

#RU_TYPES of cards to include
#FRU = field replaceable unit, CRU = customer replaceable unit
my %RU_TYPES = (FRU => 1, CRU => 1);

#Chips that are modeled as modules (card-chip together)
my %MODULE_TYPES = (PROC => 1, GPU => 1);

#Returns an array of hashes that represents the inventory
#for a system.  The hash elements are:
#TARGET:  The MRW target of the item
#OBMC_NAME: The OpenBMC name for the item.  This is usually
#           a simplified version of the target.
sub getInventory
{
    my $targetObj = shift;
    my @inventory;

    if (ref($targetObj) ne "Targets") {
        die "Invalid Targets object passed to getInventory\n";
    }

    findItems($targetObj, \@inventory);

    pruneModuleCards($targetObj, \@inventory);

    makeOBMCNames($targetObj, \@inventory);

    return @inventory;
}


#Finds the inventory targets in the MRW.
#It selects them if the target's type is in %TYPES
#or the target's RU_TYPE is in %RU_TYPES.
#This will pick up FRUs and other chips like the BMC and processor.
sub findItems
{
    my ($targetObj, $inventory) = @_;

    for my $target (sort keys %{$targetObj->getAllTargets()}) {
        my $type = "";
        my $ruType = "";

        if (!$targetObj->isBadAttribute($target, "TYPE")) {
            $type = $targetObj->getAttribute($target, "TYPE");
        }

        if (!$targetObj->isBadAttribute($target, "RU_TYPE")) {
            $ruType = $targetObj->getAttribute($target, "RU_TYPE");
        }

        if ((exists $TYPES{$type}) || (exists $RU_TYPES{$ruType})) {
            my %item;
            $item{TARGET} = $target;
            $item{OBMC_NAME} = $target; #Will fixup later
            push @$inventory, { %item };
        }
    }
}


#Removes entries from the inventory for the card target of a module.
#Needed because processors and GPUs are modeled as a package which
#is a card-chip instance that plugs into a connector on the
#backplane/processor card.  Since we already include the chip target
#in the inventory (that's how we can identify what it is), we don't
#need the entry for the card target.
#
#For example, we'll already have .../module-0/proc-0 so we don't
#need a separate .../module-0 entry.
sub pruneModuleCards
{
    my ($targetObj, $inventory) = @_;
    my @toRemove;

    #Find the parent (a card) of items of type %type
    for my $item (@$inventory) {

        if (exists $MODULE_TYPES{$targetObj->getType($item->{TARGET})}) {
            my $card = $targetObj->getTargetParent($item->{TARGET});
            push @toRemove, $card;
        }
    }

    #Remove these parent cards
    for my $c (@toRemove) {
        for my $i (0 .. (scalar @$inventory) - 1) {
            if ($c eq $inventory->[$i]{TARGET}) {
                splice(@$inventory, $i, 1);
                last;
            }
        }
    }
}


#Makes the OpenBMC name for the targets in the inventory.
#Removes unnecessary segments of the path name, renames
#some segments to match standard conventions, and numbers
#segments based on their position attribute.
sub makeOBMCNames
{
    my ($targetObj, $inventory) = @_;

    #Remove connector segments from the OBMC_NAME
    removeConnectors($targetObj, $inventory);

    #Don't need the card instance of a PROC/GPU module
    removeModuleFromPath($targetObj, $inventory);

    #Don't need card segments for non-FRUs
    removeNonFRUCardSegments($targetObj, $inventory);

    #Don't need to show the middle units between proc & core
    removeIntermediateUnits($targetObj, $inventory);

    #Certain parts have standard naming
    renameSegmentWithType("PROC", "cpu", $targetObj, $inventory);
    renameSegmentWithType("SYS", "system", $targetObj, $inventory);
    renameSegmentWithType("NODE", "chassis", $targetObj, $inventory);

    #Make sure the motherboard is called 'motherboard' in the OBMC_NAME.
    #The only way to identify it is with its TARGET_TYPE,
    #which is different than the regular type.
    renameSegmentWithTargetType("card-motherboard", "motherboard",
                                $targetObj, $inventory);

    #Don't need instance numbers unless there are more than 1 present
    removeInstNumIfOneInstPresent($inventory);

    #We want card1, not card-1
    removeHyphensFromInstanceNum($inventory);

    pointChassisAtMotherboard($targetObj, $inventory);
}


#Removes non-FRU cards in the middle of a hierarchy from OBMC_NAME.
#For example, .../motherboard/fanriser-0/fan-0 ->
# .../motherboard/fan-0 when fanriser-0 isn't a FRU.
sub removeNonFRUCardSegments
{
    my ($targetObj, $inventory) = @_;

    for my $item (@$inventory) {

        #Split the target into segments, then start
        #adding segments in to make new targets so we can
        #make API calls on the segment instances.
        my @segments = split('/', $item->{TARGET});
        my $target = "";
        for my $s (@segments) {
            next if (length($s) == 0);

            $target .= "/$s";

            my $class = $targetObj->getAttribute($target, "CLASS");
            next if ($class ne "CARD");

            my $ruType = $targetObj->getAttribute($target, "RU_TYPE");

            #If this segment is a card but not a FRU,
            #remove it from the path.
            if (not exists $RU_TYPES{$ruType}) {
                my $segment = $targetObj->getInstanceName($target);
                $item->{OBMC_NAME} =~ s/\b$segment-\d+\b\///;
            }
        }
    }
}


#Removes connectors from the OBMC_NAME element.  Also
#takes the POSITION value of the connector and adds it
#to the card segment that plugs into the connector.
#For example:
#  /motherboard/card-conn-5/card-0 ->
#  /motherobard/card-5
sub removeConnectors
{
    my ($targetObj, $inventory) = @_;

    #Find the connectors embedded in the segments
    for my $item (@$inventory) {

        #Split the target into segments, then start
        #adding segments in to make new targets
        my @segments = split('/', $item->{TARGET});
        my $target = "";
        for my $s (@segments) {
            next if (length($s) == 0);

            $target .= "/$s";
            my $class = $targetObj->getAttribute($target, "CLASS");
            next unless ($class eq "CONNECTOR");

            my ($segment) = $target =~ /\b(\w+-\d+)$/;
            my $pos = $targetObj->getAttribute($target, "POSITION");

            #change /connector-11/card-2/ to /card-11/
            $item->{OBMC_NAME} =~ s/\b$segment\/(\w+)-\d+/$1-$pos/;

        }
    }
}


#Units, typically cores, can be subunits of other subunits of
#their parent chip.  We can remove all of these intermediate
#units.  For example, cpu0/someunit1/someunit2/core3 ->
#cpu0/core3.
sub removeIntermediateUnits
{
    my ($targetObj, $inventory) = @_;

    for my $item (@$inventory) {

        my $class = $targetObj->getAttribute($item->{TARGET}, "CLASS");
        next unless ($class eq "UNIT");

        my $parent = $targetObj->getTargetParent($item->{TARGET});

        #Remove all of these intermediate units until we find
        #something that isn't a unit (most likely a chip).
        while ($targetObj->getAttribute($parent, "CLASS") eq "UNIT") {

            my $name = $targetObj->getInstanceName($parent);
            $item->{OBMC_NAME} =~ s/$name(-)*(\d+)*\///;

            $parent = $targetObj->getTargetParent($parent);
        }
    }
}


#Renames segments of the paths to the name passed in
#based on the type of the segment.
#For example:
# renameSegmentWithType("PROC", "cpu", ...);
# With a target of:
#   motherboard-0/myp9proc-5/core-3
# Where target motherboard-0/myp9proc-5 has a type
# of PROC, gives:
#   motherboard-0/cpu-5/core-3
sub renameSegmentWithType
{
    my ($type, $newSegment, $targetObj, $inventory) = @_;

    #Find the targets for all the segments, and
    #if their type matches what we're looking for, then
    #save it so we can rename them later.
    for my $item (@$inventory) {

        my @segments = split('/', $item->{TARGET});
        my $target = "";

        for my $s (@segments) {
            next if (length($s) == 0);

            $target .= "/$s";
            my $curType = $targetObj->getType($target);
            next unless ($curType eq $type);

            my $oldSegment = $targetObj->getInstanceName($target);
            $item->{OBMC_NAME} =~ s/$oldSegment/$newSegment/;
        }
    }
}


#Removes the card portion of a module from OBMC_NAME.
#For example, .../motherboard-0/module-1/proc-0 ->
#.../motherboard-0/proc-1.
#This needs to be revisited if multi-processor modules
#ever come into plan.
sub removeModuleFromPath
{
    my ($targetObj, $inventory) = @_;
    my %chipNames;

    #Find the names of the chips on the modules
    for my $item (@$inventory) {
        if (exists $MODULE_TYPES{$targetObj->getType($item->{TARGET})}) {
            $chipNames{$targetObj->getInstanceName($item->{TARGET})} = 1;
        }
    }

    #Now convert module-A/name-B to name-A
    #Note that the -B isn't always present
    for my $item (@$inventory) {

        for my $name (keys %chipNames) {
            $item->{OBMC_NAME} =~ s/\w+-(\d+)\/$name(-\d+)*/$name-$1/;
        }
    }
}


#The same as renameSegmentWithType, but finds the segment
#to rename by calling Targets::getTargetType() on it
#instead of Targets::getType().
sub renameSegmentWithTargetType
{
    my ($type, $newSegment, $targetObj, $inventory) = @_;

    for my $item (@$inventory) {

        my @segments = split('/', $item->{TARGET});
        my $target = "";

        for my $s (@segments) {
            next if (length($s) == 0);

            $target .= "/$s";
            my $curType = $targetObj->getTargetType($target);
            next unless ($curType eq $type);

            my $oldSegment = $targetObj->getInstanceName($target);
            $item->{OBMC_NAME} =~ s/$oldSegment/$newSegment/;
        }
    }
}


#FRU management code needs the node/chassis item to point
#to the motherboard and not /system/chassis.  Can revisit this
#for multi-chassis systems if they ever show up.
sub pointChassisAtMotherboard
{
    my ($targetObj, $inventory) = @_;
    my $newName = undef;

    for my $item (@$inventory) {
        my $type = $targetObj->getTargetType($item->{TARGET});
        if ($type eq "card-motherboard") {
            $newName = $item->{OBMC_NAME};
            last;
        }
    }

    for my $item (@$inventory) {
        if ($targetObj->getType($item->{TARGET}) eq "NODE") {
            if (defined $newName) {
                $item->{OBMC_NAME} = $newName;
            }
            last;
        }
    }
}


#Removes the instance number from the OBMC_NAME segments
#where only 1 of those segments exists because numbering isn't
#necessary to distinguish them.
sub removeInstNumIfOneInstPresent
{
    my ($inventory) = @_;
    my %instanceHash;

    for my $item (@$inventory) {
        #Look at all the segments, keeping track if we've
        #seen a particular segment with the same instance before.
        my @segments = split('/', $item->{OBMC_NAME});
        for my $segment (@segments) {
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

    #Remove the instanc numbers we don't need to keep.
    for my $segment (keys %instanceHash) {
        if (not exists $instanceHash{$segment}{keep}) {
            for my $item (@$inventory) {
               $item->{OBMC_NAME} =~ s/$segment-\d+/$segment/;
            }
        }
    }
}


#Removes the '-' from between the segment name and instance.
sub removeHyphensFromInstanceNum
{
    my ($inventory) = @_;

    for my $item (@$inventory) {
        $item->{OBMC_NAME} =~ s/-(\d+)\b/$1/g;
    }
}

1;

=head1 NAME

Inventory

=head1 DESCRIPTION

Retrieves the OpenBMC inventory from the MRW.

The inventory contains:

=over 4

=item * The system target

=item * The chassis target(s)  (Called a 'node' in the MRW.)

=item * All targets of class CARD or CHIP that are FRUs.

=item * All targets of type PROC

=item * All targets of type CORE

=item * All targets of type BMC

=item * All targets of type GPU

=back

=head2 Notes:

The processor and GPU chips are usually modeled in the MRW as a
card->chip package that would plug into a connector on the motherboard
or other parent card.  So, even if both card and chip are marked as a FRU,
there will only be 1 entry in the inventory for both, and the MRW
target associated with it will be for the chip and not the card.

In addition, this intermediate card will be removed from the path name:
  /system/chassis/motheboard/cpu and not
  /system/chassis/motherboard/cpucard/cpu

=head2 Inventory naming conventions

The inventory names returned in the OBMC_NAME hash element will follow
the conventions listed below.  An example of an inventory name is:
/system/chassis/motherboard/cpu5

=over 4

=item * If there is only 1 instance of any segment in the system, then
        it won't have an instance number, otherwise there will be one.

=item * The root of the name is '/system'.

=item * After system is 'chassis', of which there can be 1 or more.

=item * The name is based on the MRW card plugging order, and not what
        the system looks like from the outside.  For example, a power
        supply that plugs into a motherboard (maybe via a non-fru riser
        or cable, see the item below), would be:
        /system/chassis/motherboard/psu2 and not
        /system/chassis/psu2.

=item * If a card is not a FRU so isn't in the inventory itself, then it
        won't show up in the name of any child cards that are FRUs.
        For example, if fan-riser isn't a FRU, it would be
        /system/chassis/motherboard/fan3 and not
        /system/chassis/motherboard/fan-riser/fan3.

=item * The MRW models connectors between cards, but these never show up
        in the inventory name.

=item * If there is a motherboard, it is always called 'motherboard'.

=item * Processors, GPUs, and BMCs are always called: 'cpu', 'gpu' and
        'bmc' respectively.

=back

=head1 METHODS

=over 4

=item getInventory (C<TargetsObj>)

Returns an array of hashes representing inventory items.

The Hash contains:

* TARGET: The MRW target of the item, for example:

    /sys-0/node-0/motherboard-0/proc_socket-0/module-0/p9_proc_m

* OBMC_NAME: The OpenBMC name of the item, for example:

   /system/chassis/motherboard/cpu2

=back

=cut
