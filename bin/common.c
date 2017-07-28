#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "common.h"

void bail_out(const char* msg)
{
	perror(msg);
	exit(-1 * errno);
}

int str2int(const char* arg, int *failure_flag)
{
	long val;
	char *end;

	val = strtol(arg, &end, 10);
	/* upon successful conversion, must point to string end */
	if (failure_flag)
		*failure_flag = *arg == '\0' || *end != '\0' ||
		                val > INT_MAX || val < INT_MIN;
	return (int) val;
}

double str2double(const char* arg, int *failure_flag)
{
	double val;
	char *end;

	val = strtod(arg, &end);
	/* upon successful conversion, must point to string end */
	if (failure_flag)
		*failure_flag = *arg == '\0' || *end != '\0';
	return val;
}

char* strsplit(char split_char, char *str)
{
	char *found = strrchr(str, split_char);
	if (found) {
		/* terminate first string and move to next char */
		*found++ = '\0';
	}
	return found;
}

/*
 * returns the character that made processing stop, newline or EOF
 */
static int skip_to_next_line(FILE *fstream)
{
	int ch;	for (ch = fgetc(fstream); ch != EOF && ch != '\n'; ch = fgetc(fstream));
	return ch;
}

/*
 * skip lines that start with '#'
 */
static void skip_comments(FILE *fstream)
{
	int ch;
	for (ch = fgetc(fstream); ch == '#'; ch = fgetc(fstream))
		skip_to_next_line(fstream);
	ungetc(ch, fstream);
}

double* csv_read_column(const char *file, int column, int *num_rows)
{
	FILE *fstream;
	int  cur_row, cur_col, ch;
	double *values = NULL;
	*num_rows = 0;

	fstream = fopen(file, "r");
	if (!fstream)
		bail_out("could not open execution time file");

	/* figure out the number of jobs */
	do {
		skip_comments(fstream);
		ch = skip_to_next_line(fstream);
		if (ch != EOF)
			++(*num_rows);
	} while (ch != EOF);

	if (-1 == fseek(fstream, 0L, SEEK_SET))
		bail_out("rewinding file failed");

	/* allocate space for exec times */
	values = calloc(*num_rows, sizeof(double));
	if (!values)
		bail_out("couldn't allocate memory");

	for (cur_row = 0; cur_row < *num_rows && !feof(fstream); ++cur_row) {

		skip_comments(fstream);

		for (cur_col = 1; cur_col < column; ++cur_col) {
			/* discard input until we get to the column we want */
			int unused __attribute__ ((unused)) = fscanf(fstream, "%*s,");
		}

		/* get the desired exec. time */
		if (1 != fscanf(fstream, "%lf", values + cur_row)) {
			fprintf(stderr, "invalid execution time near line %d\n",
					cur_row);
			exit(EXIT_FAILURE);
		}

		skip_to_next_line(fstream);
	}

	assert(cur_row == *num_rows);
	fclose(fstream);

	return values;
}
