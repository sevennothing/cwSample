#########################################################################
# File Name: do.sh
# Author: ma6174
# Created Time: 2016年05月12日 星期四 15时18分03秒
#########################################################################
#!/bin/bash

killall cws

make clean

make

sudo chown root:root cws

sudo chmod +s cws

sudo pulseaudio -k ; sudo pulseaudio -D --log-target=syslog

sudo ./cws -n 2
