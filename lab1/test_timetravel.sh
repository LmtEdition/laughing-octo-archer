#!/bin/bash

#RUN TEST 2

START=$(date +%s)

    ./timetrash test2.sh

END=$(date +%s)

TIME=$(($END - $START))


START_T=$(date +%s)

    ./timetrash -t test2.sh

END_T=$(date +%s)

TIME_T=$(($END_T - $START_T))

DIFF=$(($TIME_T - $TIME))

echo "Execution took: $TIME seconds"

echo "Time travel took: $TIME_T seconds"

(("$TIME_T" <  "$TIME")) && echo "Time travel is faster"

#RUN TEST 3

START_2=$(date +%s)

    ./timetrash test3.sh

END_2=$(date +%s)

TIME_2=$(($END_2 - $START_2))


START_T_2=$(date +%s)

    ./timetrash -t test3.sh

END_T_2=$(date +%s)

TIME_T_2=$(($END_T_2 - $START_T_2))

DIFF_2=$(($TIME_T_2 - $TIME_2))

echo "Execution took: $TIME_2 seconds"

echo "Time travel took: $TIME_T_2 seconds"

rm output
(("$TIME_T_2" <=  "$TIME_2")) && echo "Time travel is faster"

#RUN TEST 4

START_3=$(date +%s)

    ./timetrash test4.sh

END_3=$(date +%s)

TIME_3=$(($END_3 - $START_3))


START_T_3=$(date +%s)

    ./timetrash -t test4.sh

END_T_3=$(date +%s)

TIME_T_3=$(($END_T_3 - $START_T_3))

DIFF_3=$(($TIME_T_3 - $TIME_3))

echo "Execution took: $TIME_3 seconds"

echo "Time travel took: $TIME_T_3 seconds"

rm out out.txt
(("$TIME_T_3" <=  "$TIME_3")) && echo "Time travel is faster" && exit
#exits with failure if slower
exit 1
