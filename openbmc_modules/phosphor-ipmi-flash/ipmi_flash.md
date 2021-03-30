# BMC-side state machine details

***ONLY ONE BLOB IS ALLOWED OPEN AT A TIME***

To avoid excessive complications, there is a restriction that only one blob
within the firmware handler can be open at a time.

The system starts in the `notYetStarted` state.

At each state, each method has a specific effect, depending on a variety of
details, and sometimes the behavior is consistent regardless of overall state.

Opening the active image or hash always fails:

*   `open(/flash/active/image)` returns false -- file cannot be opened.
*   `open(/flash/active/hash)` returns false -- file cannot be opened.

The two files are only present once their corresponding blob has been opened.

# The state of fileOpen() per state

You can only open one file at a time, and some of the states exist only when a
file is open.

State                   | fileOpen()
:---------------------- | :---------
`notYetStarted`         | `false`
`uploadInProgress`      | `true`
`verificationPending`   | `false`
`verificationStarted`   | `true`
`verificationCompleted` | `true`
`updatePending`         | `false`
`updateStarted`         | `true`
`updatedCompleted`      | `true`

# The states in the Firmware Handler State Machine

## `notYetStarted`

**The starting state.**

*   `open(/flash/image)`
*   `open(/flash/tarball)`
*   `open(/flash/hash)`
*   `open(/flash/verify)`
*   `open(/flash/update)`

## `uploadInProgress`

**The BMC is expecting to receive bytes.**

*   `open(/flash/*)` returns false because `fileOpen() == true`

*   `close(/flash/*)` triggers `state -> verificationPending`

*   `commit(/flash/*)` returns false

## `verificationPending`

**The BMC is ready for verification or more bytes.**

*   `open(/flash/image)` triggers `state -> uploadInProgress`
*   `open(/flash/tarball)` triggers `state -> uploadInProgress`
*   `open(/flash/hash)` triggers `state -> uploadInProgress`

*   `open(/flash/verify)`

*   `open(/flash/update)`

*   `commit(/flash/verify)` `state -> verificationStarted`

## `verificationStarted`

**The verification process has started, no more writes allowed.**

*   `open(/flash/*)` returns false because `state == verificationStarted`

*   `close(/flash/verify)` `state -> verificationCompleted`

## `verificationCompleted`

**The verification process has completed.**

*   `open(/flash/image)`
*   `open(/flash/tarball)`
*   `open(/flash/hash)`
*   `open(/flash/verify)`
*   `open(/flash/update)`

## `updatePending`

**The update process is pending.**

## `updateStarted`

**The update process has started.**

*   `open(/flash/*)`r eturns false because `fileOpen() == true`

## `updatedCompleted`

**The update has completed (optional state to reach)**

# Expected State Transition Sequence

If verification fails, the state still transitions to `verificationCompleted`
and similarly, if the update fails the state still transitions to
`updateCompleted`. It is up to the host-tool to check the result of the process
by running the `stat()` command on their open session (either their session with
the `verifyBlobId` or the `updateBlobId`.

Action                     | Before | After
:------------------------- | :----: | :----------------:
1. `getBlobList()`         | NYS    | NYS
2. `stat(/flash/image)`    | NYS    | NYS
3. `open(/flash/image)`    | NYS    | UIP
4. `write(...)`            | UIP    | UIP
5. `close(/flash/image)`   | UIP    | VP
6. `open(/flash/hash)`     | VP     | UIP
7. `write(...)`            | UIP    | UIP
8. `close(/flash/hash)`    | UIP    | VP
9. `open(/flash/verify)`   | VP     | VP
10. `commit(...)`          | VP     | VS
11. `sessionStat(...)`     | VS     | VS (if !completed)
11. `sessionStat(...)`     | VS     | VC (if completed)
12. `close(/flash/verify)` | VC     | UP
13. `open(/flash/update)`  | UP     | UP
14. `commit(...)`          | UP     | US
15. `sessionStat(...)`     | US     | US (if !completed)
15. `sessionStat(...)`     | US     | UC (if completed)
16. `close(/flash/update)` | UC     | NYS
