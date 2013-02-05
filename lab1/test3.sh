#!/bin/sh

# Second  command does not need to wait for First  command because of RR
echo  Hello <  output && sleep 1
echo < output &&  sleep 1 
echo Hello World  && sleep 1 
