#!/bin/sh

MODULE="201811299_dev"
MAJOR=$(awk "\$2==\"$MODULE\" {print \$1}" /proc/devices)
mknod /dev/$MODULE c $MAJOR 0
chmod 777 /dev/$MODULE
