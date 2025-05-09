#define MRZ_PARSER_IMPLEMENTATION
#include "mrzparser.h"

#include <stdio.h>

int main(void) {
	MRZ mrz;
	char line[1024];
	while (fgets(line, sizeof(line), stdin)) {
		if (!parse_mrz(&mrz, line)) {
			fprintf(stderr, "FAILED:\n%s\n", line);
			fprintf(stderr, "ERRORS:\n");
			for (int *e = mrz.errors; *e; ++e) {
				fprintf(stderr, "* %s\n", mrz_error_string(*e));
			}
			return -1;
		}
	}
	return 0;
}
