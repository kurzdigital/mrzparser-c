# MRZ Parser for C/C++

Single-file header library to parse a [Machine Readable Zone][mrz]
(Type 1, 2 and 3, [MRV][mrv] A and B, [France ID][france] and
[Swiss driver license][swiss]).

## How to use

Before you include the header file in *one* C or C++ file, you need to define
`MRZ_PARSER_IMPLEMENTATION` to create the implementation:

	#define MRZ_PARSER_IMPLEMENTATION
	#include "mrzparser.h"

## How to parse a MRZ

Then invoke `parse_mrz()` with a MRZ (the string may contain white space
and/or line breaks):

	MRZ mrz;
	char line[1024];
	if (!parse_mrz(&mrz, "I<UTOERIKSSON<<ANNA<MARIA<<<<…")) {
		fprintf(stderr, "error: %s\n", mrz.error);
	}

Now the structure `mrz` holds the parsed values from the MRZ:

	printf("document number: %s\n", mrz.document_number);

If `parse_mrz()` was unsuccessful (returned 0), `mrz` may still contain
some data depending on the nature of the error. It is guaranteed that
all members of the `mrz` struct are always null-terminated even in case
of an error.

[mrz]: https://en.wikipedia.org/wiki/Machine-readable_passport
[mrv]: https://en.wikipedia.org/wiki/Machine-readable_passport#Machine-readable_visas
[france]: https://en.wikipedia.org/wiki/National_identity_card_(France)
[swiss]: https://www.sg.ch/content/dam/sgch/verkehr/strassenverkehr/fahreignungsabkl%C3%A4rungen/informationen/Kreisschreiben%20ASTRA%20Schweiz.%20FAK.pdf
