#!/bin/sh

# 2nd command must wait on 1st  command because there is a RW conflict
echo  Hello > output && sleep 1
echo < output &&  sleep 1 
echo Hello World  && sleep 1 
exit
