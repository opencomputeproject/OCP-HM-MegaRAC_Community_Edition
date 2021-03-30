Virtual PNOR Functionality
==========================

In the abstract, the virtual PNOR function shifts mboxd away from accessing raw
flash to dynamically presenting a raw flash image to the host from a set of
backing files.

Enabling the feature virtualises both the host's access to the flash (the mbox
protocol), and the BMC's access to the flash (via some filesystem on top of the
flash device).

Do I want to use this feature in my platform?
---------------------------------------------

Maybe. It depends on how the image construction is managed, particularly the
behaviour around writes from the host. It is likely the scheme will prevent
firmware updates from being correctly applied when using flash tools on the
host.

Use-case Requirements
---------------------

Currently, the virtual PNOR implementation requires that:

* The host expect an FFS layout (OpenPOWER systems)
* The BMC provide a directory tree presenting the backing files in a hierarchy
  that reflects the partition properties in the FFS table of contents.

Implementation Behavioural Properties
-------------------------------------

1.  The FFS ToC defines the set of valid access ranges in terms of partitions
2.  The read-only property of partitions is enforced
3.  The ToC is considered read-only
4.  Read access to valid ranges must be granted
5.  Write access to valid ranges may be granted
6.  Access ranges that are valid may map into a backing file associated with
    the partition
7.  A read of a valid access range that maps into the backing file will render
    the data held in the backing file at the appropriate offset
8.  A read of a valid access range that does not map into the backing file will
    appear erased
9.  A read of an invalid access range will appear erased
10. A write to a valid access range that maps into the backing file will update
    the data in the file at the appropriate offset
11. A write to a valid access range that does not map into the backing file
    will expand the backing file to accommodate the write.
12. A write to a valid access range may fail if the range is not marked as
    writeable. The error should be returned in response to the request to open
    the write window intersecting the read-only range.
13. A write of an invalid access range will return an error. The error should
    be returned in response to the request to open the write window covering
    the invalid range.

The clarification on when the failure occurs in points 11 and 12 is useful for
host-side error handling. Opening a write window gives the indication that
future writes are expected to succeed, but in both cases we define them as
always failing. Therefore we should not give the impression to the host that
what it is asking for can be satisfied.
