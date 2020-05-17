#!/bin/bash
#ve_device="/dev/ttyUSB9"
#config TTY
#stty -F /dev/ttyUSB9 speed 19200 raw echo
ivt_command="/opt/tracer/ivtcontroler.sh"
nc_command="/opt/tracer/nc"
#bmv_statistic="/opt/tracer/readbmv_statistic.sh"
keep_reading=true
#statistic_update=100 #Read bmv statistic every x Loop

#abs() return absolute value
#abs() { 
#    [[ $[ $@ ] -lt 0 ]] && echo "$[ ($@) * -1 ]" || echo "$[ $@ ]"
#    }

declare -i online=0
declare -i old_online=0
declare -i state=0
declare -i old_state=0
while [[ $keep_reading ]]
do 

sleep 2

echo "Reading Input Voltage"
invol=`$ivt_command invol`
if [ $? -eq 0 ]
then
echo "Reading Input Voltage done"
online=1
else
echo "Error Reading Input Voltage"
online=0
fi

echo "Reading Output Voltage"
outvol=`$ivt_command outvol`
if [ $? -eq 0 ]
then
echo "Reading Output Voltage done"
online=1
if [ $outvol -gt 0 ]
then
state=1
else
state=0
fi
else
echo "Error Reading Output Voltage"
online=0
fi

echo "Reading Power"
power=`$ivt_command power`
if [ $? -eq 0 ]
then
echo "Reading Power done"
online=1
else
echo "Error Reading Power"
online=0
fi

if [ $online -eq 1 ]
then
echo "Input Voltage = $invol"
echo "Output Voltage = $outvol"
echo "Power = $power"

#Set invol
echo -e "setreg 5000 $invol" | $nc_command 127.0.0.1 1111 > /dev/null

#Set outvol
echo -e "setreg 5001 $outvol" | $nc_command 127.0.0.1 1111 > /dev/null

#Set power
echo -e "setreg 5002 $power" | $nc_command 127.0.0.1 1111 > /dev/null

#set status online
echo -e "setcoil 20 1" | $nc_command 127.0.0.1 1111 > /dev/null
if [ $outvol -gt 0 ]
then
echo -e "setcoil 21 1" | $nc_command 127.0.0.1 1111 > /dev/null
else
echo -e "setcoil 21 0" | $nc_command 127.0.0.1 1111 > /dev/null
fi

else
#set status offline
echo -e "setcoil 20 0" | $nc_command 127.0.0.1 1111 > /dev/null
#set on/standby to 0
echo -e "setcoil 21 0" | $nc_command 127.0.0.1 1111 > /dev/null
#set values to 0
echo -e "setreg 5000 0" | $nc_command 127.0.0.1 1111 > /dev/null
echo -e "setreg 5001 0" | $nc_command 127.0.0.1 1111 > /dev/null
echo -e "setreg 5002 0" | $nc_command 127.0.0.1 1111 > /dev/null
fi
if [ $old_online -ne $online ]
then
echo -e "triggerserverrealtimeupdate" | $nc_command 127.0.0.1 1111 > /dev/null
echo "Online Changed Trigger Update"
fi
if [ $old_state -ne $state ]
then
echo -e "triggerserverrealtimeupdate" | $nc_command 127.0.0.1 1111 > /dev/null
echo "State Changed Trigger Update"
fi
old_online=$online
old_state=$state
done 