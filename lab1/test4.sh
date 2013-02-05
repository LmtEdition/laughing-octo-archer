#! /bin/sh

ls && sleep 2

echo haha && sleep 2

echo first first > out.txt && sleep 2
cat out.txt && sleep 2

echo hello world > out && sleep 2
cat out && sleep 2

echo sec out.txt > out.txt && sleep 2
cat out.txt && sleep 2

echo second out > out  && sleep 2
cat out && sleep 2

echo arbok | rev && sleep 2
echo ekans | rev && sleep 2
