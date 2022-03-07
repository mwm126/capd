#!/usr/bin/bash

# Opens/forwards connections from SRC to HOST:PORT for DELAY seconds
# It is intended to be called by the authProcess of capd.

SRC=$1
HOST=$2
PORT=$3
DELAY=$4
DST=$5
IPT="/usr/sbin/iptables"
SS="/usr/sbin/ss"
connectionCount=`$SS -n|grep ":ESTAB"|grep "$HOST:$PORT "|grep -c "$SRC"`

# Open/Forward SSH port from $SRC to $HOST
$IPTABLES -A FORWARD -p tcp -s $SRC --dport $PORT -d $HOST -j ACCEPT

ticks=$DELAY*4

# Wait for new connection to occur or timer to run out
while (( ticks ))
do
  sleep 0.25
  (( ticks-- ))
  newCount=`$SS -n|grep ":ESTAB"|grep "$HOST:$PORT "|grep -c "$SRC"`
  if (( newCount == connectionCount + 1)); then
    (( ticks=0 ))
  fi
done

# Close SSH port from $SRC
$IPTABLES -D FORWARD -p tcp -s $SRC --dport $PORT -d $HOST -j ACCEPT
