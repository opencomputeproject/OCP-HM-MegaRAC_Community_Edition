#!/bin/bash

# this script checks the gpio id and loads the correct baseboard fru
fruFile="/etc/fru/baseboard.fru.bin"

cd /etc/fru
   cat aowanda.fru.bin > $fruFile
