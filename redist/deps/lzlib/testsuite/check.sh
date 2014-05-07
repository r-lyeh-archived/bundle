#! /bin/sh
# check script for Lzlib - A compression library for lzip files
# Copyright (C) 2009, 2010, 2011, 2012, 2013 Antonio Diaz Diaz.
#
# This script is free software: you have unlimited permission
# to copy, distribute and modify it.

LC_ALL=C
export LC_ALL
objdir=`pwd`
testdir=`cd "$1" ; pwd`
LZIP="${objdir}"/minilzip
BBEXAMPLE="${objdir}"/bbexample
LZCHECK="${objdir}"/lzcheck
framework_failure() { echo "failure in testing framework" ; exit 1 ; }

if [ ! -x "${LZIP}" ] ; then
	echo "${LZIP}: cannot execute"
	exit 1
fi

if [ -d tmp ] ; then rm -rf tmp ; fi
mkdir tmp
cd "${objdir}"/tmp

cat "${testdir}"/test.txt > in || framework_failure
in_lz="${testdir}"/test.txt.lz
fail=0

printf "testing lzlib-%s..." "$2"

"${LZIP}" -cqs-1 in > /dev/null
if [ $? = 1 ] ; then printf . ; else fail=1 ; printf - ; fi
"${LZIP}" -cqs0 in > /dev/null
if [ $? = 1 ] ; then printf . ; else fail=1 ; printf - ; fi
"${LZIP}" -cqs4095 in > /dev/null
if [ $? = 1 ] ; then printf . ; else fail=1 ; printf - ; fi
"${LZIP}" -cqm274 in > /dev/null
if [ $? = 1 ] ; then printf . ; else fail=1 ; printf - ; fi
"${LZIP}" -tq in
if [ $? = 2 ] ; then printf . ; else fail=1 ; printf - ; fi
"${LZIP}" -tq < in
if [ $? = 2 ] ; then printf . ; else fail=1 ; printf - ; fi
"${LZIP}" -cdq in
if [ $? = 2 ] ; then printf . ; else fail=1 ; printf - ; fi
"${LZIP}" -cdq < in
if [ $? = 2 ] ; then printf . ; else fail=1 ; printf - ; fi
dd if="${in_lz}" bs=1 count=6 2> /dev/null | "${LZIP}" -tq
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi
dd if="${in_lz}" bs=1 count=20 2> /dev/null | "${LZIP}" -tq
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi

"${LZIP}" -t "${in_lz}" || fail=1
"${LZIP}" -cd "${in_lz}" > copy || fail=1
cmp in copy || fail=1
printf .

"${LZIP}" -t "${testdir}"/test_sync.lz || fail=1
"${LZIP}" -cd "${testdir}"/test_sync.lz > copy || fail=1
cmp in copy || fail=1
printf .

"${LZIP}" -cfq "${in_lz}" > out
if [ $? = 1 ] ; then printf . ; else fail=1 ; printf - ; fi
"${LZIP}" -cF "${in_lz}" > out || fail=1
"${LZIP}" -cd out | "${LZIP}" -d > copy || fail=1
cmp in copy || fail=1
printf .

for i in s4Ki 0 1 2 3 4 5 6 7 8s16 9s16 ; do
	"${LZIP}" -k -$i in || fail=1
	mv -f in.lz copy.lz || fail=1
	printf "garbage" >> copy.lz || fail=1
	"${LZIP}" -df copy.lz || fail=1
	cmp in copy || fail=1
done
printf .

for i in s4Ki 0 1 2 3 4 5 6 7 8s16 9s16 ; do
	"${LZIP}" -c -$i in > out || fail=1
	printf "g" >> out || fail=1
	"${LZIP}" -cd out > copy || fail=1
	cmp in copy || fail=1
done
printf .

for i in s4Ki 0 1 2 3 4 5 6 7 8s16 9s16 ; do
	"${LZIP}" -$i < in > out || fail=1
	"${LZIP}" -d < out > copy || fail=1
	cmp in copy || fail=1
done
printf .

for i in s4Ki 0 1 2 3 4 5 6 7 8s16 9s16 ; do
	"${LZIP}" -f -$i -o out < in || fail=1
	"${LZIP}" -df -o copy < out.lz || fail=1
	cmp in copy || fail=1
done
printf .

"${LZIP}" < in > anyothername || fail=1
"${LZIP}" -d anyothername || fail=1
cmp in anyothername.out || fail=1
printf .

cat in in > in2 || framework_failure
"${LZIP}" < in2 > out2 || fail=1
"${LZIP}" -d < out2 > copy2 || fail=1
cmp in2 copy2 || fail=1
printf .

"${BBEXAMPLE}" in || fail=1
"${BBEXAMPLE}" out || fail=1
printf .

"${LZCHECK}" in || fail=1
printf .

echo
if [ ${fail} = 0 ] ; then
	echo "tests completed successfully."
	cd "${objdir}" && rm -r tmp
else
	echo "tests failed."
fi
exit ${fail}
