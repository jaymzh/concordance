#!/bin/bash

LOG=/dev/null
PID_FILE=/var/run/dnsmasq_concordance.pid
NM_WAIT_COUNT=3
LOCAL_IP="169.254.1.1"
REMOTE_IP="169.254.1.2"
NETMASK="255.255.0.0"
NMCLI=$(which nmcli)

check_nm() {
    IS_RUNNING=0
    if [ ! -z "$NMCLI" ] && [ -x "$NMCLI" ] ; then
        NM_STATUS=$($NMCLI -t -f RUNNING nm)
        if [ "$NM_STATUS" = "running" ]; then
            IS_RUNNING=1
        fi
    fi
    return $IS_RUNNING
}

if [ "$ACTION" = "add" ]; then
    check_nm
    if [ "$?" -eq 1 ]; then
        COUNT=0
        while [ "$COUNT" -lt "$NM_WAIT_COUNT" ]; do
            $NMCLI -t -f DEVICE,STATE dev \
                | grep "^${INTERFACE}\:connecting" >>$LOG 2>&1
            if [ "$?" = "0" ]; then
                $NMCLI dev disconnect iface $INTERFACE >>$LOG 2>&1
                break
            fi
            let COUNT=COUNT+1
            sleep 1
        done
    fi
    ifconfig $INTERFACE $LOCAL_IP netmask $NETMASK >>$LOG 2>&1
    dnsmasq --port=0 --interface=$INTERFACE --bind-interfaces --leasefile-ro \
            --dhcp-range=$REMOTE_IP,$REMOTE_IP --pid-file=$PID_FILE
else
    kill $(cat $PID_FILE)
fi
