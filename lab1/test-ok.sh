#! /bin/sh

# UCLA CS 111 Lab 1b - Test that execution works properly. 

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'

true && 
  echo true

false && 
  echo false

true || 
  echo true

false || 
  echo false

echo a b c > out.txt | 
  sort 

cat < out.txt && 
  echo cat 

(false || 
  echo ORgibberish) && echo sequence

cat < /etc/passwd | 
  tr a-z A-Z | 
    sort -u > /dev/null || 
      echo sort failed!

cat < /etc/passwd | 
  tr a-z A-Z | 
    sort -u > out || 
      echo sort failed!
EOF

cat >test.exp <<'EOF'
true
false
a b c
cat
ORgibberish
sequence
EOF

../timetrash test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
