#define MRZ_PARSER_IMPLEMENTATION
#include "mrzparser.h"

#include <stdio.h>

int main(void) {
	MRZ mrz;
	char line[1024];
	while (fgets(line, sizeof(line), stdin)) {
		if (!mrz_parse(&mrz, line)) {
			fprintf(stderr, "FAILED:\n%s\nERROR: %s\n", line, mrz.error);
			return -1;
		}
	}
	return 0;
}
