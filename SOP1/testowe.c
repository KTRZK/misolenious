/*
Skrypty testowe i checklisty

Umieść testy w pliku exam_test.sh (po skompilowaniu programów):
 */

#!/bin/sh
set -e
echo "Tworzenie testdir..."
rm -rf testdir; mkdir -p testdir/sub
echo "a" > testdir/f1.txt
echo "b" > testdir/sub/f2.txt
ln -s f1.txt testdir/link1
truncate -s 2048 testdir/big.bin

echo "Test: exam_count_simple"
./exam_count_simple testdir

echo "Test: nftw_count"
./exam_nftw_count testdir

echo "Test: copy_preserve_metadata"
./copy_preserve_metadata testdir/f1.txt copied_f1
ls -l testdir/f1.txt copied_f1

echo "Test: list_dir_sorted"
./list_dir_sorted testdir

echo "Test: top_n_largest"
./top_n_largest testdir 3

echo "Cleanup"
rm -rf testdir copied_f1
