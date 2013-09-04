/*
 * Copyright: 2013 Xilinx Inc
 * Written by Edgar E. Iglesias <edgar.iglesias@xilinx.com>
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <search.h>

#include "util.h"
#include "syms.h"
#include "coverage.h"
#include "cov-gcov.h"
#include "cov-cachegrind.h"

static void coverage_dump_sym(struct sym *s, FILE *fp)
{
	uint64_t addr, end = s->addr + s->size;
	unsigned int i =  0;
	uint64_t accounted = 0;

	for (addr = s->addr; addr < end; addr += 4) {
		uint64_t v = 0;

		if (s->cov)
			v = s->cov->counter[i];
		accounted += v;

		fprintf(fp, "%" PRId64 " %" PRIx64 " %s %s:%d\n",
			v, addr, s->name ? s->name : "unknown",
			s->src_filename ? s->src_filename : "unknown",
			s->linemap ? s->linemap->locs[i].linenr : 0);
		i++;
	}
}

static void coverage_dump(struct sym *s, size_t nr_syms,
			  struct sym *unknown, FILE *fp)
{
	size_t i;

	for (i = 0; i < nr_syms; i++) {
		coverage_dump_sym(&s[i], fp);
	}
	fprintf(fp, "%" PRId64 " x unknown\n", unknown->total_time);
}

void coverage_emit(void **store, const char *filename, enum cov_format fmt,
		const char *gcov_strip, const char *gcov_prefix)
{
	FILE *fp = NULL;
	struct sym *s, *unknown;
	size_t nr_syms;

	s = sym_get_all(store, &nr_syms);
	if (!s)
		return;

	unknown = sym_get_unknown(store);

	if (filename) {
		fp = fopen(filename, "w+");
		if (!fp) {
			perror(filename);
			return;
		}
	}

	switch (fmt) {
	case ETRACE:
		coverage_dump(s, nr_syms, unknown, fp);
		break;
	case CACHEGRIND:
		cachegrind_coverage_dump(s, nr_syms, unknown, fp);
		break;
	default:
		gcov_emit_gcov(store, s, nr_syms, unknown, fp,
				gcov_strip, gcov_prefix, fmt);
		break;
	}

	if (fp) {
		fflush(fp);
		fclose(fp);
	}
}

void coverage_init(void **store, const char *filename, enum cov_format fmt,
		const char *gcov_strip, const char *gcov_prefix)
{
}
