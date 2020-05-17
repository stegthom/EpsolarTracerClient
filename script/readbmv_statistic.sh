#!/bin/bash
ve_device="/dev/ttyUSB9"
#config TTY
#stty -F /dev/ttyUSB9 speed 19200 raw echo
ve_command="/opt/tracer/bmvcontroler"
nc_command="/opt/tracer/nc"
keep_reading=true

#abs() return absolute value
abs() { 
    [[ $[ $@ ] -lt 0 ]] && echo "$[ ($@) * -1 ]" || echo "$[ $@ ]"
    }


#while [[ $keep_reading ]]
#do 

#sleep 1

while [ $? != 1 ]
do
echo "Reading deepestdiscarge"
deepestdischarge=`$ve_command -d $ve_device -g a`
done
echo "Reading deepestdischarge done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading lastdischarge"
lastdischarge=`$ve_command -d $ve_device -g b`
done
echo "Reading lastdischarge  done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading averagedischarge"
averagedischarge=`$ve_command -d $ve_device -g c`
done
echo "Reading averagedischarge  done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading cycles"
cycles=`$ve_command -d $ve_device -g d`
done
echo "Reading cycles  done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading fulldischarges"
fulldischarges=`$ve_command -d $ve_device -g e`
done
echo "Reading fulldicharges done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading cumulativeah"
cumulativeah=`$ve_command -d $ve_device -g f`
done
echo "Reading cumulativeah done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading minvoltage"
minvoltage=`$ve_command -d $ve_device -g g`
done
echo "Reading minvoltage done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading maxvoltage"
maxvoltage=`$ve_command -d $ve_device -g h`
done
echo "Reading maxvoltage  done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading secsincefull"
secsincefull=`$ve_command -d $ve_device -g i`
done
echo "Reading secsincefull  done" #Needed to Reset the $?!


while [ $? != 1 ]
do
echo "Reading autosyncs"
autosyncs=`$ve_command -d $ve_device -g j`
done
echo "Reading autosyncs  done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading lowalarms"
lowalarms=`$ve_command -d $ve_device -g l`
done
echo "Reading lowalarms  done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading highalarms"
highalarms=`$ve_command -d $ve_device -g m`
done
echo "Reading highalarms done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading discharged energy"
dischargedenergy=`$ve_command -d $ve_device -g r`
done
echo "Reading discharged energy  done" #Needed to Reset the $?!

while [ $? != 1 ]
do
echo "Reading chargedenergy"
chargedenergy=`$ve_command -d $ve_device -g s`
done
echo "Reading chargedenergy done" #Needed to Reset the $?!



#remove sign from deepestdischarge
deepestdischarge=`abs $deepestdischarge`

#remove sign from lastdischarge
lastdischarge=`abs $lastdischarge`

#remove sign from averagedischarge
averagedischarge=`abs $averagedischarge`

#remove sign from cumulativeah
cumulativeah=`abs $cumulativeah`

echo "deepestdischarge = $deepestdischarge"
echo "lastdischarge = $lastdischarge"
echo "averagedischarge = $averagedischarge"
echo "cycles = $cycles"
echo "fulldischarges = $fulldischarges"
echo "cumulativeah = $cumulativeah"
echo "minvoltage = $minvoltage"
echo "maxvoltage = $maxvoltage"
echo "secsincefull = $secsincefull"
echo "autosyncs = $autosyncs"
echo "lowalarms = $lowalarms"
echo "highalarms = $highalarms"
echo "dischargedenergy = $dischargedenergy"
echo "chargedenergy = $chargedenergy"


#Set deepestdischarge
echo -e "setreg 7000 $deepestdischarge" | $nc_command 127.0.0.1 1111 > /dev/null

#Set lastdischarge
echo -e "setreg 7001 $lastdischarge" | $nc_command 127.0.0.1 1111 > /dev/null

#Set averagedischarge
echo -e "setreg 7002 $averagedischarge" | $nc_command 127.0.0.1 1111 > /dev/null

#set cycles
echo -e "setreg 7003 $cycles" | $nc_command 127.0.0.1 1111 > /dev/null

#Set fulldischarges
echo -e "setreg 7004 $fulldischarges" | $nc_command 127.0.0.1 1111 > /dev/null

#Set cumulativeah
echo -e "setreg32 7005 $cumulativeah" | $nc_command 127.0.0.1 1111 > /dev/null

#Set minvoltage
echo -e "setreg 7007 $minvoltage" | $nc_command 127.0.0.1 1111 > /dev/null

#Set maxvoltage
echo -e "setreg 7008 $maxvoltage" | $nc_command 127.0.0.1 1111 > /dev/null

#Set secsincefull
echo -e "setreg 7009 $secsincefull" | $nc_command 127.0.0.1 1111 > /dev/null

#Set autosyncs
echo -e "setreg 700A $autosyncs" | $nc_command 127.0.0.1 1111 > /dev/null

#Set lowalarms
echo -e "setreg 700B $lowalarms" | $nc_command 127.0.0.1 1111 > /dev/null

#Set highalarms
echo -e "setreg 700C $highalarms" | $nc_command 127.0.0.1 1111 > /dev/null

#Set dischargedenergy
echo -e "setreg32 7010 $dischargedenergy" | $nc_command 127.0.0.1 1111 > /dev/null

#Set chargedenergy
echo -e "setreg32 7012 $chargedenergy" | $nc_command 127.0.0.1 1111 > /dev/null

#done 