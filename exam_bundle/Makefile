CC = gcc
CFLAGS = -Wall -Wextra -std=c11
XOPEN = -D_XOPEN_SOURCE=700

all: exam_count_simple exam_nftw_count copy_lowlevel copy_stdio make_file scanner_filters listing_program writev_example sop-venv find_broken_symlinks copy_preserve_metadata recursive_delete group_by_extension tail_n_lines atomic_replace safe_temp_write lock_modify_requirements read_range

exam_count_simple: exam_count_simple.c util.h
	$(CC) $(CFLAGS) exam_count_simple.c -o exam_count_simple

exam_nftw_count: exam_nftw_count.c util.h
	$(CC) $(CFLAGS) $(XOPEN) exam_nftw_count.c -o exam_nftw_count

copy_lowlevel: copy_lowlevel.c util.h
	$(CC) $(CFLAGS) copy_lowlevel.c -o copy_lowlevel

copy_stdio: copy_stdio.c util.h
	$(CC) $(CFLAGS) copy_stdio.c -o copy_stdio

make_file: make_file.c util.h
	$(CC) $(CFLAGS) make_file.c -o make_file

scanner_filters: scanner_filters.c util.h
	$(CC) $(CFLAGS) $(XOPEN) scanner_filters.c -o scanner_filters

listing_program: listing_program.c util.h
	$(CC) $(CFLAGS) listing_program.c -o listing_program

writev_example: writev_example.c util.h
	$(CC) $(CFLAGS) writev_example.c -o writev_example

sop-venv: sop-venv.c util.h
	$(CC) $(CFLAGS) sop-venv.c -o sop-venv

find_broken_symlinks: find_broken_symlinks.c util.h
	$(CC) $(CFLAGS) $(XOPEN) find_broken_symlinks.c -o find_broken_symlinks

copy_preserve_metadata: copy_preserve_metadata.c util.h
	$(CC) $(CFLAGS) copy_preserve_metadata.c -o copy_preserve_metadata

recursive_delete: recursive_delete.c util.h
	$(CC) $(CFLAGS) $(XOPEN) recursive_delete.c -o recursive_delete

group_by_extension: group_by_extension.c util.h
	$(CC) $(CFLAGS) $(XOPEN) group_by_extension.c -o group_by_extension

tail_n_lines: tail_n_lines.c util.h
	$(CC) $(CFLAGS) tail_n_lines.c -o tail_n_lines

atomic_replace: atomic_replace.c util.h
	$(CC) $(CFLAGS) atomic_replace.c -o atomic_replace

safe_temp_write: safe_temp_write.c util.h
	$(CC) $(CFLAGS) safe_temp_write.c -o safe_temp_write

lock_modify_requirements: lock_modify_requirements.c util.h
	$(CC) $(CFLAGS) lock_modify_requirements.c -o lock_modify_requirements

read_range: read_range.c util.h
	$(CC) $(CFLAGS) read_range.c -o read_range

dir_diff: dir_diff.c util.h
	$(CC) $(CFLAGS) $(XOPEN) dir_diff.c -o dir_diff

clean:
	rm -f exam_count_simple exam_nftw_count copy_lowlevel copy_stdio make_file scanner_filters listing_program writev_example sop-venv find_broken_symlinks copy_preserve_metadata recursive_delete group_by_extension tail_n_lines atomic_replace safe_temp_write lock_modify_requirements read_range dir_diff sample.bin sample.copy myenv listing.txt writev_out.txt

.PHONY: all clean
