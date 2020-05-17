#!/bin/bash

#getcommand="wget http://192.168.1.100/inv_dat.shtml?3 -q -O - | egrep -o '[-+]?([0-9]*\.[0-9]+|[0-9]+)'"
getaddress="http://192.168.1.100/inv_dat.shtml?3"
onaddress="http://192.168.1.100/iload.shtml?31"
offaddress="http://192.168.1.100/iload.shtml?30"
invdata=(`wget $getaddress -t 2 -T 2 -w 1 -q -O - | egrep -o '[-+]?([0-9]*\.[0-9]+|[0-9]+)'`)

if [[ $? != 0 ]]
then
exit -1
fi

invdata[0]=`echo ${invdata[0]} |sed 's/\.//g'`

case "$1" in
  invol)
      echo ${invdata[0]}
      ;;
  outvol)
      echo ${invdata[1]}
      ;;
  power)
      echo ${invdata[2]}
      ;;
  all)
      echo ${invdata[*]}
      ;;
  on)
      wget -t 2 -T 2 -w 1 -q -O - $onaddress
      if [[ $? != 0 ]]
      then
      exit -1
      fi
      exit 0
      ;;
  off)
      wget -t 2 -T 2 -w 1 -q -O - $offaddress
      if [[ $? != 0 ]]
      then
      exit -1
      fi
      exit 0
      ;;
				    
  
   *)
     echo "Usage: ivtcontroler.sh {invol|outvol|power|all|on|off}"
     exit 2
     ;;
     esac
						       
