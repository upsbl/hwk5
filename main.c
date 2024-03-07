#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "rtclock.h"
#include "mmm.h"

// shared  globals
unsigned int mode;
unsigned int size, num_threads;
double **A, **B, **SEQ_MATRIX, **PAR_MATRIX;

static char *SEQ_USAGE = "%s%s S <size>\n";
static char *PAR_USAGE = "%s%s P <num threads> <size>\n";
static char *TOO_MANY_THREADS = "Cannot distribute %d rows over more than %d threads\n";
static char *BANNER = "========\nmode: %s\nthread count: %u\nsize: %u\n========\n";

static int getint(char *s, unsigned int *p);
static int doargs(int argc, char **argv);
static double doseq();
static void dopar();

static int getint(char *s, unsigned int *p)
{
	int n;
	n = atoi(s);
	if (n < 1)
		return -1;
	*p = n;
	return 0;
}

static int doargs(int argc, char **argv)
{
	if (argc < 2) goto argfail;
	if (strcmp(argv[1], "S") == 0) {
		mode = 0;
		num_threads = 1;
		if (argc != 3) goto sfail;
		if (getint(argv[2], &size) < 0) goto sfail;
		return 0;
	sfail:
		fprintf(stderr, SEQ_USAGE, "Usage: ", argv[0]);
		return -1;
	} else if (strcmp(argv[1], "P") == 0) {
		mode = 1;
		if (argc != 4) goto pfail;
		if (getint(argv[2], &num_threads) < 0) goto pfail;
		if (getint(argv[3], &size) < 0) goto pfail;
		if (num_threads > size) {
			fprintf(stderr, TOO_MANY_THREADS, size, size);
			return -1;
		}
		return 0;
	pfail:
		fprintf(stderr, PAR_USAGE, "Usage: ", argv[0]);
		return -1;
	}
argfail:
	fprintf(stderr, SEQ_USAGE, "Usage:\t", argv[0]);
	fprintf(stderr, PAR_USAGE, "\t", argv[0]);
	return -1;
}

static double doseq()
{
	double start, avg;
	int i;
	avg = 0.0;
	for (i=0; i<3; i++) {
		start = rtclock();
		mmm_seq();
		avg += rtclock() - start;
	}
	avg /= 3.0;
	printf("Sequential Time (avg of 3 runs): %f sec\n", avg);
	return avg;
}

static void dopar()
{
	double seq, start, avg, diff;
	int i;
	seq = doseq();
	avg = 0.0;
	for (i=0; i<3; i++) {
		start = rtclock();
		mmm_par();
		avg += rtclock() - start;
	}
	avg /= 3.0;
	printf("Parallel Time (avg of 3 runs): %f sec\n", avg);
	printf("Speedup: %f\n", seq/avg);
	printf("Verifying... ");
	diff = mmm_verify();
	printf("largest error between parallel and sequential matrix: %f\n", diff);
}

int main(int argc, char *argv[])
{
	if (doargs(argc, argv) < 0)
		return 0;
	printf(BANNER, mode?"parallel":"sequential", num_threads, size);
	mmm_init();
	if (mode == 0)
		doseq();
	else
		dopar();
	mmm_freeup();
	return 0;
}
