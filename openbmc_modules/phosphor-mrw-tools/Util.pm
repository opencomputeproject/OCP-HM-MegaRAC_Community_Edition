package Util;

#Holds common utility functions for MRW processing.

use strict;
use warnings;

use mrw::Targets;

#Returns the BMC target for a system.
# param[in] $targetObj = The Targets object
sub getBMCTarget
{
    my ($targetObj) = @_;

    for my $target (keys %{$targetObj->getAllTargets()}) {
        if ($targetObj->getType($target) eq "BMC") {
           return $target;
        }
    }

    die "Could not find BMC target in the MRW XML\n";
}


#Returns an array of child units based on their Target Type.
# param[in] $targetObj = The Targets object
# param[in] $unitTargetType = The target type of the units to find
# param[in] $chip = The chip target to find the units on
sub getChildUnitsWithTargetType
{
    my ($targetObj, $unitTargetType, $chip) = @_;
    my @units;

    my @children = $targetObj->getAllTargetChildren($chip);

    for my $child (@children) {
        if ($targetObj->getTargetType($child) eq $unitTargetType) {
            push @units, $child;
        }
    }

    return @units;
}

#Returns size of child units based on their Type.
# param[in] $targetObj = The Targets object
# param[in] $type = The type of the units to find
# param[in] $chip = The chip target to find the units on
sub getSizeOfChildUnitsWithType
{
    my ($targetObj, $type, $chip) = @_;
    my @units;
    my @children = $targetObj->getAllTargetChildren($chip);
    for my $child (@children) {
        if ($targetObj->getType($child) eq $type) {
            push @units, $child;
        }
    }
    my $size = @units;
    return $size;
}

# Returns OBMC name corresponding to a Target name
# param[in] \@inventory = reference to array of inventory items
# param[in] $targetName = A Target name
sub getObmcName
{
    my ($inventory_ref, $targetName) = @_;
    my @inventory = @{ $inventory_ref };

    for my $item (@inventory)
    {
        if($item->{TARGET} eq $targetName)
        {
            return $item->{OBMC_NAME};
        }
    }
    return undef;
}

#Returns the array of all the device paths based on the type.
# param[in] \@inventory = reference to array of inventory items.
# param[in] $targetObj = The Targets object.
# param[in] $type = The target type.
sub getDevicePath
{
    my ($inventory_ref, $targetObj, $type) = @_;
    my @inventory = @{ $inventory_ref };
    my $fruType = "";
    my @devices;
    for my $item (@inventory)
    {
        if (!$targetObj->isBadAttribute($item->{TARGET}, "TYPE")) {
            $fruType = $targetObj->getAttribute($item->{TARGET}, "TYPE");
            if ($fruType eq $type) {
                push(@devices,$item->{OBMC_NAME});
            }
        }
    }
    return @devices;
}

#Convert the MRW I2C address into the standard 7-bit format
# $addr = the I2C Address
sub adjustI2CAddress
{
    my $addr = shift;

    #MRW holds the 8 bit value.  We need the 7 bit one.
    $addr = $addr >> 1;
    $addr = sprintf("0x%X", $addr);
    $addr = lc $addr;

    return $addr;
}

#Return nearest FRU enclosing the input target
# $targets = the Targets object
# $targetName = the target name
sub getEnclosingFru
{
    my ($targets, $targetName) = @_;

    if (($targetName eq "") || (!defined $targetName))
    {
        return "";
    }

    if (!$targets->isBadAttribute($targetName, "RU_TYPE"))
    {
        my $ruType = $targets->getAttribute($targetName, "RU_TYPE");
        if (($ruType eq "FRU") || ($ruType eq "CRU"))
        {
            return $targetName;
        }
    }

    my $parent = $targets->getTargetParent($targetName);
    return getEnclosingFru($targets, $parent);
}

#Convert I2C port number from MRW scheme to Linux numbering scheme
# $port = the I2C port number
sub adjustI2CPort
{
    my $port = shift;

    return $port - 1;
}

# Get the location code for the target passed in, like P1-C5.
#  $targets = the targets object
#  $target = target to get the location code of
sub getLocationCode
{
    my ($targets, $passedInTarget) = @_;
    my $locCode = undef;
    my $target = $passedInTarget;
    my $done = 0;

    # Walk up the parent chain prepending segments until an absolute
    # location code is found.
    while (!$done)
    {
        if (!$targets->isBadAttribute($target, "LOCATION_CODE"))
        {
            my $loc = $targets->getAttribute($target, "LOCATION_CODE");
            my $type = $targets->getAttribute($target, "LOCATION_CODE_TYPE");

            if (length($loc) > 0 )
            {
                if (defined $locCode)
                {
                    $locCode = $loc . '-' . $locCode;
                }
                else
                {
                    $locCode = $loc;
                }

                if ($type eq "ABSOLUTE")
                {
                    $done = 1;
                }
            }
            else
            {
                die "Missing location code on $target\n" if $type eq "ABSOLUTE";
            }
        }

        if (!$done)
        {
            $target = $targets->getTargetParent($target);
            if (not defined $target)
            {
                die "Did not find complete location " .
                    "code for $passedInTarget\n";
            }
        }
    }

    return $locCode;
}

1;

=head1 NAME

Util

=head1 DESCRIPTION

Contains utility functions for the MRW parsers.

=head1 METHODS

=over 4

=item getBMCTarget(C<TargetsObj>)

Returns the target string for the BMC chip.  If it can't find one,
it will die.  Currently supports single BMC systems.

=item getChildUnitsWithTargetType(C<TargetsObj>, C<TargetType>, C<ChipTarget>)

Returns an array of targets that have target-type C<TargetType>
and are children (any level) of target C<ChipTarget>.

=item getSizeOfChildUnitsWithType(C<TargetsObj>, C<Type>, C<ChipTarget>)

Returns size of targets that have Type C<Type>
and are children (any level) of target C<ChipTarget>.

=item getObmcName(C<InventoryItems>, C<TargetName>)

Returns an OBMC name corresponding to a Target name. Returns
undef if the Target name is not found.

=item getDevicePath(C<InventoryItems>, C<TargetsObj>, C<TargetType>)

Returns an array of all device paths (OBMC names)  based on C<TargetType>.

=item adjustI2CAddress(C<I2CAddress>)

Returns C<I2CAddress> converted from MRW format (8-bit) to the standard 7-bit
format.

=item getEnclosingFru(C<TargetsObj>, C<Target>)

Finds the nearest FRU enclosing the input Target.

=item adjustI2CPort(C<I2CPort>)

Returns C<I2CPort> converted from MRW numbering scheme to Linux numbering scheme.

=item getLocationCode(C<TargetsObj, C<Target>)

Gets the location code for the input Target.

=back

=cut
