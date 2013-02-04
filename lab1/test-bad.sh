#! /bin/sh

# UCLA CS 111 Lab 1b - Test that execution works properly. 

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'

ahegiealg
mv hageilha hagiehlag
ls -agehialg
cat < eagaegag
sort ahegiagehl
diff aeghil agheiaghi
rm aeghieahl
EOF

cat >test.exp <<'EOF'
No such file or directory
mv: cannot stat `hageilha': No such file or directory
ls: invalid option -- 'e'
Try `ls --help' for more information.
No such file or directory
sort: open failed: ahegiagehl: No such file or directory
diff: aeghil: No such file or directory
diff: agheiaghi: No such file or directory
rm: cannot remove `aeghieahl': No such file or directory
EOF

../timetrash test.sh 2>test.out || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
