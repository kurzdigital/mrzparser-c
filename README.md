# MRZ Parser for C/C++

Single-file header library to parse a [Machine Readable Zone][mrz]
(Type 1, 2 and 3, [MRV][mrv] A and B, and France ID).

## How to use

Before you include the header file in *one* C or C++ file, you need to define
`MRZ_PARSER_IMPLEMENTATION` to create the implementation:

	#define MRZ_PARSER_IMPLEMENTATION
	#include "mrzparser.h"

## How to parse a MRZ

Then invoke `mrz_parse()` with a MRZ (the string may contain white space
and/or line breaks):

	MRZ mrz;
	char line[1024];
	if (!mrz_parse(&mrz, "I<UTOERIKSSON<<ANNA<MARIA<<<<…")) {
		fprintf(stderr, "error: %s\n", mrz.error);
	}

Now the structure `mrz` holds the parsed values from the MRZ:

	printf("document number: %s\n", mrz.document_number);

If `mrz_parse()` was unsuccessful (returned 0), `mrz` may still contain
some data depending on the nature of the error. It is guaranteed that
all members of the `mrz` struct are always null-terminated even in case
of an error.

[mrz]: https://en.wikipedia.org/wiki/Machine-readable_passport
[mrv]: https://en.wikipedia.org/wiki/Machine-readable_passport#Machine-readable_visas
