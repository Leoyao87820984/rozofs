#!/bin/bash
#  Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
#  This file is part of Rozofs.
#  Rozofs is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published
#  by the Free Software Foundation, version 2.
#  Rozofs is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.

#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see
#  <http://www.gnu.org/licenses/>.

VFSTAT=/tmp/vfstat_rozodebug.$$
VERSION="Version 1.0"
PROGNAME=`basename $0`

# Exit codes
STATE_OK=0
STATE_WARNING=1
STATE_CRITICAL=2
STATE_UNKNOWN=3

ROZODEBUG_PATHS=". /usr/bin /usr/local/bin $ROZO_TESTS/build/src/debug" 
resolve_rozodebug() {

  option="-i $host -p $port"

  if [ ! -z "$time" ];
  then
    option=`echo "$option -t $time"`
  fi

  for path in $ROZODEBUG_PATHS
  do
    if [ -f $path/rozodebug ];
    then
      ROZDBG="$path/rozodebug $option"
      return
    fi  
  done
  echo "Can not find rozodebug"
  exit $STATE_UNKNOWN 
}

# Help functions
print_revision() {
   # Print the revision number
   echo "$PROGNAME - $VERSION"
}
print_usage() {
   # Print a short usage statement
   echo "Usage: $PROGNAME [-v] [-p <port>] [-t <seconds>] -H <host> [-volume <volume#>] -w <percent%> -c <percent%>"
}
print_help() {

   # Print detailed help information
   print_revision
   print_usage

  echo "Options:"
  echo "{-h|--help}"
  echo "    Print detailed help screen"
  echo "{-V|--version}"
  echo "    Print version information"
  echo "-volume <volume#>"
  echo "    Volume number to monitor"  
  echo "{-H|--hostname} <host name>"
  echo "    Destination host of the export daemon"
  echo "{-t|--timeout} <seconds>"
  echo "    Time out value"  
  echo "{-w|--warning} <threshold|percent%>"
  echo "    Exit with WARNING status if less than <threshold> GB or <percent>% of disk is free"
  echo "{-c|--critical} <threshold|percent%>"
  echo "    Exit with CRITICAL status if less than <threshold> GB or <percent>% of disk is free"
  echo "-p <port>"
  echo "    To eventually redefine the export debug port"
  echo "{-v|--verbose}"
  echo "    Verbose output"
  exit $STATE_OK
}

display_output() {
  case $1 in
   "$STATE_OK")         echo "OK <> FREE $free $percent%";;
   "$STATE_WARNING")    echo "WARNING <$2> FREE $free $percent%";;
   "$STATE_CRITICAL")   echo "CRITICAL <$2> FREE $free $percent%";; 
   "$STATE_UNKNOWN")    echo "UNKNOWN <$2> FREE $free $percent%";;    
  esac
  rm -f $VFSTAT
  exit $1
}
set_default() {
  verbosity=0
  thresh_warn=""
  thresh_warn_percent=""
  thresh_crit=""
  thresh_crit_percent=""
  host=""
  time=""
  port=50000
  free=0
  percent=0
  volume=1
}

is_numeric () {
  if [ "$(echo $1 | grep "^[ [:digit:] ]*$")" ] 
  then 
    value=$1
    return 1
  fi
  return 0
}
is_percent () {
  last=`echo $1 | tail -c2`
  if [ $last != '%' ] 
  then 
    return 0
  fi
      
  value=`echo $1 | sed 's/.\{1\}$//g'`
  is_numeric $value
  if [ $? = 0 ]
  then
    display_output $STATE_UNKNOWN "Percent value is not numeric"    
  fi
  if [ $value -gt 100 ];
  then
    display_output $STATE_UNKNOWN "Percent must not exceed 100"
  fi
  if [ $value -lt 0 ];
  then
    display_output $STATE_UNKNOWN "Percent must not be less than 0"
  fi
  return 1 
}

scan_numeric_value () {
  if [ -z "$2" ];
  then 
      display_output $STATE_UNKNOWN "Option $1 requires an argument"   
  fi
  is_numeric $2
  if [ $? = 0 ];
  then
      display_output $STATE_UNKNOWN "Option $1 requires a numeric argument"   	   	
  fi
  value=$2
}
scan_value () {
  if [ -z "$2" ];
  then 
      display_output $STATE_UNKNOWN "Option $1 requires an argument"   
  fi
  value=$2
}


date > /var/tmp/rozo
echo "call $0 $1 $2 $3 $4 $5 $6 $7 $8 $9" >> /var/tmp/rozo


#######################################################
#         M A I N
#######################################################

# Set default values
set_default


# Parse command line options
while [ "$1" ]; do
   case "$1" in
       -h | --help)           print_help;;
       -V | --version)        print_revision; exit $STATE_OK;;
       -v | --verbose)        set -x; verbosity=1; shift 1;;
       -w | --warning | -c | --critical) {
           if [ -z "$2" ];
	   then 
               display_output $STATE_UNKNOWN "Option $1 requires an argument"   
	   fi
	   is_percent $2
	   if [ $? = 1 ];
	   then
	     [[ "$1" = *-w* ]] && thresh_warn_percent=$value || thresh_crit_percent=$value  	       
	   else
	     is_numeric $2
	     if [ $? = 1 ];
	     then
	       [[ "$1" = *-w* ]] && thresh_warn=$value || thresh_crit=$value  	     
	     else
               display_output $STATE_UNKNOWN "Threshold must be integer or percentage"
             fi
	   fi
           shift 2
        };;
	-H | --hostname)  scan_value $1 $2;         host=$value;       shift 2;;	
	-volume)          scan_numeric_value $1 $2; volume=$value;     shift 2;;	
	-t | --time)      scan_numeric_value $1 $2; time=$value;       shift 2;;
	-p)               scan_numeric_value $1 $2; port=$value;       shift 2;;
       *) display_output $STATE_UNKNOWN "Unexpected parameter or option $1"
   esac
done


# Check mandatory parameters are set

if [ -z $host ];
then
   display_output $STATE_UNKNOWN "-H option is mandatory"
fi  

if [ -z "$thresh_warn" ];
then
  if [ -z "$thresh_warn_percent" ];
  then
    display_output $STATE_UNKNOWN "-w option is mandatory"
  fi     
fi  


if [ -z "$thresh_crit" ];
then
  if [ -z "$thresh_crit_percent" ];
  then
    display_output $STATE_UNKNOWN "-c option is mandatory"
  fi     
fi  


# ping the destination host

ping $host -c 1 >> /dev/null
if [ $? != 0 ]
then
  # re attempt a ping
  ping $host -c 2 >> /dev/null
  if [ $? != 0 ]
  then  
    display_output $STATE_CRITICAL "$host do not respond to ping"
  fi 
fi


# Find rozodebug utility and prepare command line parameters

resolve_rozodebug


# Run vfstat_vol debug command on export to get volume statistics

$ROZDBG -c vfstat_vol >  $VFSTAT
res=`grep "Volume:" $VFSTAT`
case $res in
  "") {
    display_output $STATE_CRITICAL "$host:$port do not respond to rozodebug vfstat_vol"
  };;  
esac


# Extract volume usage from the debug output

res=`awk '{if (($1=="Volume:") && ($2==volume)) printf("%s %s\n",$8,$10);}' volume=$volume $VFSTAT`
case $res in 
  "") display_output $STATE_CRITICAL "$host do not host volume $volume"
esac
free=`echo $res | awk '{print $1}'`
percent=`echo $res | awk '{print $2}'`


# Run vfstat_stor debug command on export to check storage status

$ROZDBG -c vfstat_stor >  $VFSTAT
res=`grep "Vid" $VFSTAT`
case $res in
  "") {
    display_output $STATE_CRITICAL "$host do not respond to rozodebug vfstat_stor"
  };;  
esac

up=`awk 'BEGIN {nb=0;} {if (($1==volume) && ($5=="UP")) nb++;} END {printf("%d\n",nb);}' volume=$volume $VFSTAT`
if [ $up -eq 0 ]
then
  display_output $STATE_CRITICAL "No storage is UP"
fi
down=`awk 'BEGIN {nb=0;} {if (($1==volume) && ($5=="DOWN")) nb++;} END {printf("%d\n",nb);}' volume=$volume $VFSTAT`
if  [ $down -gt 0 ] 
then
  display_output $STATE_WARNING "$down storages are down"
fi


# Check if critical threshold is excedeed

if [ ! -z $thresh_crit  ];
then
  if [ $free -le $thresh_crit ];
  then
    display_output $STATE_CRITICAL "Free volume threshold less than $thresh_cri"    
  fi
else 
  if [ $percent -le $thresh_crit_percent ];
  then
    display_output $STATE_CRITICAL "Free volume threshold less than $thresh_crit_percent%"    
  fi
fi  

# Check if warning threshold is excedeed

if [ ! -z $thresh_warn  ];
then 
  if [ $free -le $thresh_warn ];
  then
    display_output $STATE_WARNING "Free volume threshold less than $thresh_warn"    
  fi
else 
  if [ $percent -le $thresh_warn_percent ];
  then
    display_output $STATE_WARNING "Free volume threshold less than $thresh_warn_percent%"    
  fi
fi  

# Hurra !!!
display_output $STATE_OK 
