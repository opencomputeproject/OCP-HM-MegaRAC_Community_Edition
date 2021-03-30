Description
===========

The trace script automates the process of enabling named sets of tracepoints
and dumping the trace file over the SSH connection, then cleaning up after
itself.

Usage
=====

To run:

```
Usage: ./trace [USER@]HOST EVENTSET [EVENTSET...]

Valid EVENTSETs: fsi, occ, sbefifo, timer, sched

```

To stop the trace output and disable the tracepoints, hit `Return`.

For example, capturing the defined tracepoints for the `sched` event set:

```
$ ./trace root@my-bmc sched
+ set -eou pipefail
+ set -x
+ EVENT_fsi='fsi fsi_master_gpio'
+ EVENT_occ='occ hwmon_occ'
+ EVENT_sbefifo=sbefifo
+ EVENT_timer='timer/timer_start timer/timer_cancel timer/timer_expire_entry timer/timer_expire_exit'
+ EVENT_sched='sched/sched_switch sched/sched_wakeup sched/sched_wakeup_new sched/sched_waking'
+ trap on_exit EXIT
+ CAT_PID=1867
+ for elem in sched
+ eval 'trace=\${EVENT_${elem}}'
++ trace='${EVENT_sched}'
++ eval echo '${EVENT_sched}'
+++ echo sched/sched_switch sched/sched_wakeup sched/sched_wakeup_new sched/sched_waking
+ for event in '$(eval echo ${trace})'
+ echo 1
+ for event in '$(eval echo ${trace})'
+ echo 1
+ for event in '$(eval echo ${trace})'
+ echo 1
+ for event in '$(eval echo ${trace})'
+ echo 1
+ echo 1
+ read
+ cat /sys/kernel/debug/tracing/per_cpu/cpu0/trace_pipe
              sh-1866  [000] d.h. 201821.030000: sched_waking: comm=rngd pid=987 prio=120 target_cpu=000
              sh-1866  [000] dnh. 201821.030000: sched_wakeup: comm=rngd pid=987 prio=120 target_cpu=000
              sh-1866  [000] dn.. 201821.030000: sched_waking: comm=ksoftirqd/0 pid=6 prio=120 target_cpu=000
              sh-1866  [000] dn.. 201821.030000: sched_wakeup: comm=ksoftirqd/0 pid=6 prio=120 target_cpu=000
              sh-1866  [000] d... 201821.030000: sched_switch: prev_comm=sh prev_pid=1866 prev_prio=120 prev_state=R ==> next_comm=rngd next_pid=987 next_prio=120
            rngd-987   [000] d... 201821.030000: sched_switch: prev_comm=rngd prev_pid=987 prev_prio=120 prev_state=S ==> next_comm=ksoftirqd/0 next_pid=6 next_prio=120
     ksoftirqd/0-6     [000] d... 201821.030000: sched_switch: prev_comm=ksoftirqd/0 prev_pid=6 prev_prio=120 prev_state=S ==> next_comm=sh next_pid=1866 next_prio=120
              sh-1866  [000] d.h. 201821.030000: sched_waking: comm=phosphor-hwmon- pid=1188 prio=120 target_cpu=000
              sh-1866  [000] dnh. 201821.030000: sched_wakeup: comm=phosphor-hwmon- pid=1188 prio=120 target_cpu=000
              sh-1866  [000] d... 201821.030000: sched_switch: prev_comm=sh prev_pid=1866 prev_prio=120 prev_state=R ==> next_comm=phosphor-hwmon- next_pid=1188 next_prio=120
 phosphor-hwmon--1188  [000] d... 201821.030000: sched_switch: prev_comm=phosphor-hwmon- prev_pid=1188 prev_prio=120 prev_state=D ==> next_comm=sh next_pid=1866 next_prio=120
...
<RETURN>
...
+ kill 1867
+ on_exit
+ for elem in sched
+ eval 'trace=\${EVENT_${elem}}'
++ trace='${EVENT_sched}'
++ eval echo '${EVENT_sched}'
+++ echo sched/sched_switch sched/sched_wakeup sched/sched_wakeup_new sched/sched_waking
+ for event in '$(eval echo ${trace})'
+ echo 0
+ for event in '$(eval echo ${trace})'
+ echo 0
+ for event in '$(eval echo ${trace})'
+ echo 0
+ for event in '$(eval echo ${trace})'
+ echo 0
+ echo 0
+ on_exit
+ rm -f obmc-fsi-trace.Rz15GL
```
