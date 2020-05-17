#!/bin/bash
ve_device="/dev/ttyUSB9"
#config TTY
#stty -F /dev/ttyUSB9 speed 19200 raw echo
ve_command="/opt/tracer/bmvcontroler"
nc_command="/opt/tracer/nc"
bmv_statistic="/opt/tracer/readbmv_statistic.sh"
keep_reading=true
statistic_update=100 #Read bmv statistic every x Loop

#abs() return absolute value
abs() { 
    [[ $[ $@ ] -lt 0 ]] && echo "$[ ($@) * -1 ]" || echo "$[ $@ ]"
    }

declare -i n=0
while [[ $keep_reading ]]
do 

if [ $n -eq $statistic_update ]
then
$bmv_statistic
n=0
fi

sleep 2

while [ $? != 1 ]
do
echo "Reading SOC"
soc=`$ve_command -d $ve_device -g soc`
done
echo "Reading Soc done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading NetCurrent"
netcurrent=`$ve_command -d $ve_device -g current`
done
echo "Reading NetCurrent  done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading ConsumedAH"
consumedah=`$ve_command -d $ve_device -g consumedah`
done
echo "Reading ConsumedAH  done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading TimeToGo"
ttg=`$ve_command -d $ve_device -g ttg`
done
echo "Reading TimeToGo  done" #Needed to Reset the $?!


#soc=`grep -m 1 -w "SOC" /dev/ttyUSB9| sed 's/[^0-9]//g'`
#netcurrent=`grep -m 1 -w "I" /dev/ttyUSB9| sed 's/[^-0-9]//g'`

#echo $soc
#echo $netcurrent

soc=$(($soc/100))
netcurrent=$(($netcurrent*10))

#echo $soc
#echo $netcurrent

chargingcurrent=`echo -e "getreg 3105" | $nc_command 127.0.0.1 1111`

#echo $chargingcurrent

#if [[ ! $chargingcurrent ]]
#then
#continue
#fi

if [ "$chargingcurrent" == "Timeout" ]
then
echo "Error GetReg Timeout"
continue
fi

loadvoltage=`echo -e "getreg 310C" | $nc_command 127.0.0.1 1111`

#echo $loadvoltage

#if [[ ! $loadvoltage ]]
#then
#continue
#fi

if [ "$loadvoltage" == "Timeout" ]
then
echo "Error GetReg Timeout"
continue
fi

loadcurrent=$(($chargingcurrent - $netcurrent))
loadpower=$(($loadcurrent * ($loadvoltage / 100)))

#remove sign from consumedah
consumedah=`abs $consumedah`

echo "SOC = $soc"
echo "netcurrent = $netcurrent"
echo "consumedah = $consumedah"
echo "TimeToGo = $ttg"
echo "chargingcurrent = $chargingcurrent"
echo "loadcurrent = $loadcurrent"
echo "loadpower = $loadpower"

#Set SOC
echo -e "setreg 311A $soc" | $nc_command 127.0.0.1 1111 > /dev/null

#Set NetCurrent
echo -e "setreg32 331B $netcurrent" | $nc_command 127.0.0.1 1111 > /dev/null

#Set ConsumedAH
echo -e "setreg 4001 $consumedah" | $nc_command 127.0.0.1 1111 > /dev/null

#set TTG
echo -e "setreg 4002 $ttg" | $nc_command 127.0.0.1 1111 > /dev/null

#Set Loadcurrent
echo -e "setreg 310D $loadcurrent" | $nc_command 127.0.0.1 1111 > /dev/null

#Set LoadPower
echo -e "setreg32 310E $loadpower" | $nc_command 127.0.0.1 1111 > /dev/null
n=n+1
done 