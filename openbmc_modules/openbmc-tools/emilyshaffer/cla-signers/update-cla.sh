#!/bin/bash

# Sort the names in the CLA (discarding the header)
cat cla-signers | tail -n +2 | sort -u > cla-signers.new
# Add a new header with fresh timestamp (discarding old file)
echo Updated $(date) > cla-signers
# Add sorted names below header
cat cla-signers.new >> cla-signers
# Remove temporary file
rm cla-signers.new
