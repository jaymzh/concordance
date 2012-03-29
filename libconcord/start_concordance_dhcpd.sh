#!/bin/bash

LOG=/dev/null
PID_FILE=/var/run/dnsmasq_concordance.pid
NM_WAIT_COUNT=3
LOCAL_IP="169.254.1.1"
REMOTE_IP="169.254.1.2"
NETMASK="255.255.0.0"
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
IPTABLES_RULE="INPUT -p udp -i $INTERFACE --dport 67 -j ACCEPT"
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
        echo "NetworkManager found running." >>$LOG
        COUNT=0
        while [ "$COUNT" -lt "$NM_WAIT_COUNT" ]; do
            echo "Waiting for NetworkManager." >>$LOG
            $NMCLI -t -f DEVICE,STATE dev \
                | grep "^${INTERFACE}\:connecting" >>$LOG 2>&1
            if [ "$?" = "0" ]; then
                echo "Disconnecting $INTERFACE via NetworkManager." >>$LOG
                $NMCLI dev disconnect iface $INTERFACE >>$LOG 2>&1
                break
            fi
            let COUNT=COUNT+1
            sleep 1
        done
    fi
    echo "Configuring $INTERFACE interface." >>$LOG
    ifconfig $INTERFACE $LOCAL_IP netmask $NETMASK >>$LOG 2>&1
    echo "Adding iptables rule." >>$LOG
    iptables -I $IPTABLES_RULE >>$LOG 2>&1
    dnsmasq --port=0 --interface=$INTERFACE --bind-interfaces --leasefile-ro \
            --dhcp-range=$REMOTE_IP,$REMOTE_IP --pid-file=$PID_FILE
else
    echo "Stopping dnsmasq and removing iptables rule." >>$LOG
    kill $(cat $PID_FILE)
    iptables -D $IPTABLES_RULE >>$LOG 2>&1
fi
