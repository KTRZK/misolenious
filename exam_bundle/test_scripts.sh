#!/bin/sh
set -e
echo "Creating testdir..."
rm -rf testdir; mkdir -p testdir/sub
echo "a" > testdir/f1.txt
echo "b" > testdir/sub/f2.txt
ln -s f1.txt testdir/link1
truncate -s 1024 testdir/big.bin

echo "Compiling selected programs (make if available)..."
if [ -f Makefile ]; then make -j 4; else
    gcc -Wall -Wextra -std=c11 exam_count_simple.c -o exam_count_simple
    gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 exam_nftw_count.c -o exam_nftw_count
    gcc -Wall -Wextra -std=c11 copy_lowlevel.c -o copy_lowlevel
    gcc -Wall -Wextra -std=c11 make_file.c -o make_file
    gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 scanner_filters.c -o scanner_filters
    gcc -Wall -Wextra -std=c11 sop-venv.c -o sop-venv
    gcc -Wall -Wextra -std=c11 writev_example.c -o writev_example
fi

echo "Test: exam_count_simple"
./exam_count_simple testdir || echo "exam_count_simple failed"

echo "Test: exam_nftw_count"
./exam_nftw_count testdir || echo "exam_nftw_count failed"

echo "Test: make_file"
./make_file -n sample.bin -p 0644 -s 2000 || echo "make_file failed"
ls -l sample.bin

echo "Test: copy_lowlevel"
./copy_lowlevel sample.bin sample.copy || echo "copy_lowlevel failed"
cmp sample.bin sample.copy && echo "copy ok" || echo "copy differs"

echo "Test: scanner_filters"
./scanner_filters -p testdir -d 2 -e txt -o listing.txt || echo "scanner_filters failed"
cat listing.txt || true

echo "Test: sop-venv"
./sop-venv -c -v myenv || echo "create env failed"
./sop-venv -v myenv -i pkg==0.1 || echo "install failed"
cat myenv/requirements || true
./sop-venv -v myenv -r pkg || echo "remove failed"

echo "Done tests."
