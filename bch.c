# include <stdio.h>
# include <malloc.h>
# include <string.h>
# include <unistd.h>
# include <fcntl.h>
# include <errno.h>
# include <mdlint.h>
# include <sys/stat.h>
# include <mdl/bci.h>
# include <sys/types.h>
enum {
	_rbc_to_bch,
	_bch_to_rbc
};

# define PAGE_SIZE 12
void resize(mdl_u8_t **__itr, mdl_u8_t **__ref, mdl_uint_t *__page_c, mdl_uint_t __nbc) {
	mdl_uint_t curr_size = (*__itr)-(*__ref);

	for (;;) {
	if (curr_size+__nbc >= (*__page_c)*PAGE_SIZE) {
		*__ref = (mdl_u8_t*)realloc(*__ref, (++(*__page_c))*PAGE_SIZE);
		*__itr = (*__ref)+curr_size;
	} else break;
	}
}

# define incr_itr(__a, __b) __a+=__b;

# include <math.h>
mdl_u8_t* rbc_to_bch(mdl_u8_t *__src, mdl_uint_t __size, mdl_uint_t *__dst_size) {
	mdl_u8_t *buff = (mdl_u8_t*)malloc(PAGE_SIZE);
	mdl_uint_t page_c = 1;

	mdl_u8_t *src_itr = __src;
	mdl_u8_t *itr = buff;
	while(src_itr < __src+__size) {
		resize(&itr, &buff, &page_c, 1);
		*itr = ':';
		incr_itr(itr, 1);

		mdl_u8_t byte_c = bcii_sizeof(src_itr, *(bci_flag_t*)(src_itr+1))+bcii_overhead_size();

		resize(&itr, &buff, &page_c, 2);
		sprintf(itr, "%02X", byte_c);
		incr_itr(itr, 2);

		printf("%u\n", byte_c);
		mdl_u8_t *end = itr+(byte_c*2);
		for (;itr != end;) {
			mdl_u8_t *old_itr = itr;
			resize(&itr, &buff, &page_c, 2);
			if (old_itr != itr) {
				printf("--->%u\n", end-old_itr);
				end = itr+(end-old_itr);
			}

			sprintf(itr, "%02X", *src_itr);
			incr_itr(itr, 2);
			src_itr++;
		}

		resize(&itr, &buff, &page_c, 1);
		*itr = '\n';
		incr_itr(itr, 1);
	}

	printf("exit.\n");

	*__dst_size=itr-buff;
	return buff;
}

mdl_u8_t* bch_to_rbc(mdl_u8_t *__src, mdl_uint_t __size, mdl_uint_t *__dst_size) {
	mdl_u8_t *dst = (mdl_u8_t*)malloc(PAGE_SIZE);
	mdl_uint_t page_c = 1;

	mdl_u8_t *itr = __src;
	mdl_u8_t *dst_itr = dst;

	mdl_u8_t byte_c;
	while(itr < __src+__size) {
		if (*(itr++) != ':') {
			fprintf(stderr, "invalid starting point, got: %c\n", *(itr-1));
			goto err;
		}

		if (sscanf(itr, "%2hhx", &byte_c) != 1) {
			fprintf(stderr, "failed to read byte count.\n");
			goto err;
		} else
			printf("byte count: %u\n", byte_c);

		incr_itr(itr, 2);

		mdl_u8_t *end = itr+(byte_c*2);
		for (;itr != end;) {
			resize(&dst_itr, &dst, &page_c, 1);
			if (sscanf(itr, "%2hhx", dst_itr++) != 1) {
				fprintf(stderr, "failed to read data.\n");
				goto err;
			}

			incr_itr(itr, 2);
		}

		if (*(itr++) != '\n') {
			fprintf(stderr, "missing newline.\n");
			goto err;
		}
	}

	err:
	*__dst_size = dst_itr-dst-1;
	return dst;
}

int main(int __argc, char const *__argv[]) {
	if (__argc < 7) {
		printf("usage:\n -o - dst file.\n -i - src file.\n -c - conv kind.\n");
		return -1;
	}

	mdl_u8_t conv_kind = (mdl_u8_t)~0;
	char const *src_fpth = NULL, *dst_fpth = NULL;
	char const **arg_itr = __argv+1;
	while(arg_itr < __argv+__argc) {
		if (!strcmp(*arg_itr, "-o"))
			dst_fpth = *(++arg_itr);
		else if (!strcmp(*arg_itr, "-i"))
			src_fpth = *(++arg_itr);
		else if (!strcmp(*arg_itr, "-c")) {
			char const *s = *(++arg_itr);
			if (!strcmp(s, "rbc"))
				conv_kind = _bch_to_rbc;
			else if (!strcmp(s, "bch"))
				conv_kind = _rbc_to_bch;
		}
		arg_itr++;
	}

	if (conv_kind == (mdl_u8_t)~0 || !src_fpth || !dst_fpth) {
		fprintf(stderr, "something went wong.\n");
		return -1;
	}

	int fd;
	struct stat st;

	if ((fd = open(src_fpth, O_RDONLY)) < 0) {
		fprintf(stderr, "failed to open file at %s\n", src_fpth);
		return -1;
	}

	if (stat(src_fpth, &st) < 0) {
		close(fd);
		fprintf(stderr, "failed to stat file at %s\n", src_fpth);
		return -1;
	}

	mdl_u8_t *src = (mdl_u8_t*)malloc(st.st_size);
	read(fd, src, st.st_size);
	close(fd);

	mdl_u8_t *dst;
	mdl_uint_t dst_size = 0;
	if (conv_kind == _rbc_to_bch)
		dst = rbc_to_bch(src, st.st_size, &dst_size);
	else if (conv_kind == _bch_to_rbc)
		dst = bch_to_rbc(src, st.st_size, &dst_size);

	printf("%s - %s - %u- %c\n", src_fpth, dst_fpth, dst_size, *(src+2));

	if (access(dst_fpth, F_OK) != -1) {
		if (truncate(dst_fpth, 0) < 0) {
			fprintf(stderr, "failed to truncate file at %s\n", dst_fpth);
			goto err;
		}
	}

	if ((fd = open(dst_fpth, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_ISUID)) < 0) {
		fprintf(stderr, "failed to open file at %s\n", dst_fpth);
		goto err;
	}

	write(fd, dst, dst_size);

	err:
	close(fd);

	free(dst);
	free(src);
	return 0;
}
