#!/bin/bash

SourceModule() {
  . fan_rpm_loop_test.sh
}

SetupShims() {
  MkDir()      { echo "MkDir      $*"; }
  Mv()         { echo "Mv         $*"; }
  Sleep()      { echo "Sleep      $*"; }
  SystemCtl()  { echo "SystemCtl  $*"; }
  CommandRpm() { echo "CommandRpm $*"; }
}

TestRunRpmStepsWorks() {
  RunRpmSteps 1000 5000 3 30 || return
  RunRpmSteps 5000 1000 3 30 || return
  RunRpmSteps 1000 5000 1 30 || return
  RunRpmSteps 5000 1000 1 30 || return
}

TestMainRejectsLowMinAndMax() {
  if main 0 0; then
    echo "main 0 0 not rejected?"
    return 1
  fi
  if main 1 0; then
    echo "main 1 0 not rejected?"
    return 1
  fi
}

TestMainWorks() {
  main 1000 5005 || return
}

main() {
  SourceModule                || return
  SetupShims                  || return
  TestRunRpmStepsWorks        || return
  TestMainRejectsLowMinAndMax || return
  TestMainWorks               || return
  echo "All tests completed."
}

return 0 2>/dev/null
main "$@"

