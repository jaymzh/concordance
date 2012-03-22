#!/bin/bash

# Kicks off dhcpd startup in the background so udev can go about its work.
$(dirname $0)/start_concordance_dhcpd.sh &
