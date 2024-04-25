#ifndef __mrzparser_h_
#define __mrzparser_h_

struct MRZ {
	char document_code[3];
	char issuing_state[4];
	char primary_identifier[46];
	char secondary_identifier[46];
	char nationality[4];
	char document_number[46];
	char date_of_birth[7];
	char sex[2];
	char date_of_expiry[7];
	const char *error;
};
typedef struct MRZ MRZ;

int mrz_parse(struct MRZ *, const char *);

#ifdef MRZ_PARSER_IMPLEMENTATION
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef stpncpy
char *stpncpy(char *dst, const char *src, size_t len) {
	size_t l = strlen(src);
	if (l > len) {
		l = len;
	}
	return strncpy(dst, src, len) + l;
}
#endif

#define MRZ_CHARACTERS "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define MRZ_NUMBERS "0123456789"
#define MRZ_FILLER "<"
#define MRZ_SEXES "MFX" MRZ_FILLER
#define MRZ_ALL MRZ_CHARACTERS MRZ_NUMBERS MRZ_FILLER
#define MRZ_CHARACTERS_AND_NUMBERS MRZ_CHARACTERS MRZ_CHARACTERS
#define MRZ_CHARACTERS_AND_FILLER MRZ_CHARACTERS MRZ_FILLER
#define MRZ_NUMBERS_AND_FILLER MRZ_NUMBERS MRZ_FILLER
#define MRZ_CAPACITY(s) (sizeof(s) - 1)
#define MRZ_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MRZ_DOCUMENT_CODE "document code"
#define MRZ_ISSUING_STATE "issuing state"
#define MRZ_DOCUMENT_NUMBER "document number"
#define MRZ_DOCUMENT_NUMBER_CHECK_DIGIT "document number check digit"
#define MRZ_OPTIONAL_DATA1 "optional data1"
#define MRZ_DATE_OF_BIRTH "date of birth"
#define MRZ_DATE_OF_BIRTH_CHECK_DIGIT "date of birth check digit"
#define MRZ_SEX "sex"
#define MRZ_DATE_OF_EXPIRY "date of expiry"
#define MRZ_DATE_OF_EXPIRY_CHECK_DIGIT "date of expiry check digit"
#define MRZ_NATIONALITY "nationality"
#define MRZ_OPTIONAL_DATA2 "optional data2"
#define MRZ_PERSONAL_NUMBER "personal number"
#define MRZ_PERSONAL_NUMBER_CHECK_DIGIT "personal number check digit"
#define MRZ_COMBINED_CHECK_DIGIT "combined check digit"
#define MRZ_VISA "visa"
#define MRZ_NO_VISA "no visa"
#define MRZ_NOT_FRANCE "not france"
#define MRZ_DEPARTMENT_OF_ISSUANCE "department of issuance"
#define MRZ_OFFICE_OF_ISSUANCE "office of issuance"
#define MRZ_YEAR_OF_ISSUANCE "year of issuance"
#define MRZ_MONTH_OF_ISSUANCE "month of issuance"
#define MRZ_IDENTIFIERS "names"
#define MRZ_CSUM_DOCUMENT_NUMBER "checksum for document number"
#define MRZ_CSUM_DOB "checksum for date of birth"
#define MRZ_CSUM_DOE "checksum for date of expiry"
#define MRZ_CSUM_PERSONAL_NUMBER "checksum for personal number"
#define MRZ_CSUM_COMBINED "combined checksum"
#define MRZ_ASSERT_CSUM(x, m, e) if (!x && !m->error) { m->error = e; }

static int mrz_parser_check_digit_weights[] = {7, 3, 1};
static int mrz_parser_check(const char *digit, ...) {
	if (!digit) {
		return 0;
	}
	va_list ap;
	va_start(ap, digit);
	int sum = 0;
	int i = 0;
	for (;;) {
		const char *s = va_arg(ap, char *);
		if (!s) {
			break;
		}
		for (; *s; ++s, ++i) {
			char c = *s;
			if (c > 64 && c < 91) {
				c -= 55; // Map A-Z to 10-35.
			} else if (c > 47 && c < 58) {
				c -= 48; // Use number value.
			} else if (c == 60) {
				c = 0; // '<' is 0.
			} else {
				va_end(ap);
				return 0; // Invalid character.
			}
			sum += c * mrz_parser_check_digit_weights[i % 3];
		}
	}
	sum %= 10;
	va_end(ap);
	return sum == (*digit == '<' ? 0 : *digit - 48);
}

static int mrz_parser_check_extended(const char *digit, const char *s,
		const char *ext) {
	if (*digit == '<') {
		const char *p = strchr(ext, '<');
		size_t len = p - ext;
		if (len > 1) {
			char d[2] = {ext[--len], 0};
			// Unfortunately, Note j in ICAO 9303p5 doesn't specify
			// if the `<` shall be part of the checksum calculation
			// or not. This means some issuers include the `<` and
			// other don't so we need to accept both variants.
			char with[91] = {0};
			size_t cap_w = MRZ_CAPACITY(with);
			strncat(with, s, cap_w);
			strncat(with, "<", cap_w - strlen(with));
			if (cap_w - strlen(with) < len) {
				return 0;
			}
			strncat(with, ext, len);
			char without[91] = {0};
			size_t cap_wo = MRZ_CAPACITY(without);
			strncat(without, s, cap_wo);
			if (cap_wo - strlen(without) < len) {
				return 0;
			}
			strncat(without, ext, len);
			return mrz_parser_check(d, with, NULL) ||
					mrz_parser_check(d, without, NULL);
		}
	}
	return mrz_parser_check(digit, s, NULL);
}

static void mrz_parser_rtrim(char *s) {
	char *end = s + strlen(s);
	for (; end > s && *(end - 1) == ' '; --end);
	*end = 0;
}

static void mrz_parser_strrep(char *s, char find, char replacement) {
	char *p = s;
	while ((p = strchr(p, find))) {
		*p = replacement;
	}
}

static void mrz_parser_parse_identifiers(MRZ *mrz, const char *identifiers) {
	const char *p = strstr(identifiers, "<<");
	size_t cap = MRZ_CAPACITY(mrz->primary_identifier);
	size_t len = p - identifiers;
	if (p && len < cap) {
		cap = len;
		*stpncpy(mrz->secondary_identifier, p + 2,
				MRZ_CAPACITY(mrz->secondary_identifier)) = 0;
		mrz_parser_strrep(mrz->secondary_identifier, '<', ' ');
		mrz_parser_rtrim(mrz->secondary_identifier);
	}
	*stpncpy(mrz->primary_identifier, identifiers, cap) = 0;
	mrz_parser_strrep(mrz->primary_identifier, '<', ' ');
	mrz_parser_rtrim(mrz->primary_identifier);
}

static int mrz_parser_parse(const char **src, char *field,
		size_t len, const char *allowed,
		const char *description, const char **error) {
	if (!**src) {
		return 0;
	}
	int invalid = strspn(*src, allowed) < len;
	if (invalid) {
		if (!*error) {
			*error = description;
		}
		if (strlen(*src) < len) {
			// Move to the end of the string to make sure all
			// subsequent calls to mrz_parser_parse() will fail, too.
			for (; **src; ++*src);
			return 0;
		}
	}
	*stpncpy(field, *src, len) = 0;
	*src += len;
	return invalid ^ 1;
}

static int mrz_parser_parse_td1(MRZ *mrz, const char *s) {
	mrz->error = NULL;
	const char **e = &mrz->error;
	int success = 1;

	// First line.
	char document_number_check_digit[2] = {0};
	char optional_data1[16] = {0};
	success &= mrz_parser_parse(&s, mrz->document_code, 2, MRZ_ALL,
			MRZ_DOCUMENT_CODE, e);
	success &= mrz_parser_parse(&s, mrz->issuing_state, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_ISSUING_STATE, e);
	success &= mrz_parser_parse(&s, mrz->document_number, 9,
			MRZ_ALL, MRZ_DOCUMENT_NUMBER, e);
	success &= mrz_parser_parse(&s, document_number_check_digit, 1,
			MRZ_NUMBERS_AND_FILLER, MRZ_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, optional_data1, 15, MRZ_ALL,
			MRZ_OPTIONAL_DATA1, e);

	// Second line.
	success &= mrz_parser_parse(&s, mrz->date_of_birth, 6,
			MRZ_NUMBERS_AND_FILLER, MRZ_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_birth_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->sex, 1, MRZ_SEXES, MRZ_SEX, e);
	success &= mrz_parser_parse(&s, mrz->date_of_expiry, 6,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_expiry_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->nationality, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_NATIONALITY, e);
	char optional_data2[12] = {0};
	success &= mrz_parser_parse(&s, optional_data2, 11,
			MRZ_ALL, MRZ_OPTIONAL_DATA2, e);
	char check_digit[2] = {0};
	success &= mrz_parser_parse(&s, check_digit, 1,
			MRZ_NUMBERS, MRZ_COMBINED_CHECK_DIGIT, e);

	// Third line.
	char identifiers[31] = {0};
	success &= mrz_parser_parse(&s, identifiers, 30,
			MRZ_CHARACTERS_AND_FILLER, MRZ_IDENTIFIERS, e);
	mrz_parser_parse_identifiers(mrz, identifiers);

	// Validate check sums.
	success &= mrz_parser_check_extended(document_number_check_digit,
			mrz->document_number, optional_data1);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOCUMENT_NUMBER);
	success &= mrz_parser_check(date_of_birth_check_digit,
			mrz->date_of_birth, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOB);
	success &= mrz_parser_check(date_of_expiry_check_digit,
			mrz->date_of_expiry, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOE);
	success &= mrz_parser_check(check_digit,
			mrz->document_number,
			document_number_check_digit,
			optional_data1,
			mrz->date_of_birth,
			date_of_birth_check_digit,
			mrz->date_of_expiry,
			date_of_expiry_check_digit,
			optional_data2,
			NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_COMBINED);

	return success;
}

static int mrz_parser_parse_td2(MRZ *mrz, const char *s) {
	if (*s == 'V') {
		mrz->error = MRZ_VISA;
		return 0;
	}
	mrz->error = NULL;
	const char **e = &mrz->error;
	int success = 1;

	// First line.
	success &= mrz_parser_parse(&s, mrz->document_code, 2,
			MRZ_ALL, MRZ_DOCUMENT_CODE, e);
	success &= mrz_parser_parse(&s, mrz->issuing_state, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_ISSUING_STATE, e);
	char identifiers[32] = {0};
	success &= mrz_parser_parse(&s, identifiers, 31,
			MRZ_CHARACTERS_AND_FILLER, MRZ_IDENTIFIERS, e);
	mrz_parser_parse_identifiers(mrz, identifiers);

	// Second line.
	success &= mrz_parser_parse(&s, mrz->document_number, 9,
			MRZ_ALL, MRZ_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, document_number_check_digit, 1,
			MRZ_NUMBERS_AND_FILLER, MRZ_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->nationality, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_NATIONALITY, e);
	success &= mrz_parser_parse(&s, mrz->date_of_birth, 6,
			MRZ_NUMBERS_AND_FILLER, MRZ_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_birth_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->sex, 1, MRZ_SEXES, MRZ_SEX, e);
	success &= mrz_parser_parse(&s, mrz->date_of_expiry, 6,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_expiry_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	char optional_data2[8] = {0};
	success &= mrz_parser_parse(&s, optional_data2, 7,
			MRZ_ALL, MRZ_OPTIONAL_DATA2, e);
	char check_digit[2] = {0};
	success &= mrz_parser_parse(&s, check_digit, 1,
			MRZ_NUMBERS, MRZ_COMBINED_CHECK_DIGIT, e);

	// Validate check sums.
	success &= mrz_parser_check_extended(document_number_check_digit,
			mrz->document_number, optional_data2);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOCUMENT_NUMBER);
	success &= mrz_parser_check(date_of_birth_check_digit,
			mrz->date_of_birth, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOB);
	success &= mrz_parser_check(date_of_expiry_check_digit,
			mrz->date_of_expiry, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOE);
	success &= mrz_parser_check(check_digit,
			mrz->document_number,
			document_number_check_digit,
			mrz->date_of_birth,
			date_of_birth_check_digit,
			mrz->date_of_expiry,
			date_of_expiry_check_digit,
			optional_data2,
			NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_COMBINED);

	return success;
}

static int mrz_parser_parse_td3(MRZ *mrz, const char *s) {
	if (*s == 'V') {
		mrz->error = MRZ_VISA;
		return 0;
	}
	mrz->error = NULL;
	const char **e = &mrz->error;
	int success = 1;

	// First line.
	success &= mrz_parser_parse(&s, mrz->document_code, 2,
			MRZ_ALL, MRZ_DOCUMENT_CODE, e);
	success &= mrz_parser_parse(&s, mrz->issuing_state, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_ISSUING_STATE, e);
	char identifiers[40] = {0};
	success &= mrz_parser_parse(&s, identifiers, 39,
			MRZ_CHARACTERS_AND_FILLER, MRZ_IDENTIFIERS, e);
	mrz_parser_parse_identifiers(mrz, identifiers);

	// Second line.
	success &= mrz_parser_parse(&s, mrz->document_number, 9,
			MRZ_ALL, MRZ_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, document_number_check_digit, 1,
			MRZ_NUMBERS_AND_FILLER, MRZ_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->nationality, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_NATIONALITY, e);
	success &= mrz_parser_parse(&s, mrz->date_of_birth, 6,
			MRZ_NUMBERS_AND_FILLER, MRZ_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_birth_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->sex, 1, MRZ_SEXES, MRZ_SEX, e);
	success &= mrz_parser_parse(&s, mrz->date_of_expiry, 6,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_expiry_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	char personal_number[15] = {0};
	success &= mrz_parser_parse(&s, personal_number, 14,
			MRZ_ALL, MRZ_PERSONAL_NUMBER, e);
	char personal_number_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, personal_number_check_digit, 1,
			MRZ_NUMBERS_AND_FILLER, MRZ_PERSONAL_NUMBER_CHECK_DIGIT, e);
	char check_digit[2] = {0};
	success &= mrz_parser_parse(&s, check_digit, 1,
			MRZ_NUMBERS, MRZ_COMBINED_CHECK_DIGIT, e);

	// Validate check sums.
	success &= mrz_parser_check(document_number_check_digit,
			mrz->document_number, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOCUMENT_NUMBER);
	success &= mrz_parser_check(date_of_birth_check_digit,
			mrz->date_of_birth, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOB);
	success &= mrz_parser_check(date_of_expiry_check_digit,
			mrz->date_of_expiry, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOE);
	success &= mrz_parser_check(personal_number_check_digit,
			personal_number, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_PERSONAL_NUMBER);
	success &= mrz_parser_check(check_digit,
			mrz->document_number,
			document_number_check_digit,
			mrz->date_of_birth,
			date_of_birth_check_digit,
			mrz->date_of_expiry,
			date_of_expiry_check_digit,
			personal_number,
			personal_number_check_digit,
			NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_COMBINED);

	return success;
}

static int mrz_parser_parse_mrva(MRZ *mrz, const char *s) {
	if (*s != 'V') {
		mrz->error = MRZ_NO_VISA;
		return 0;
	}
	mrz->error = NULL;
	const char **e = &mrz->error;
	int success = 1;

	// First line.
	success &= mrz_parser_parse(&s, mrz->document_code, 2,
			MRZ_ALL, MRZ_DOCUMENT_CODE, e);
	success &= mrz_parser_parse(&s, mrz->issuing_state, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_ISSUING_STATE, e);
	char identifiers[40] = {0};
	success &= mrz_parser_parse(&s, identifiers, 39,
			MRZ_CHARACTERS_AND_FILLER, MRZ_IDENTIFIERS, e);
	mrz_parser_parse_identifiers(mrz, identifiers);

	// Second line.
	success &= mrz_parser_parse(&s, mrz->document_number, 9,
			MRZ_ALL, MRZ_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, document_number_check_digit, 1,
			MRZ_NUMBERS_AND_FILLER, MRZ_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->nationality, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_NATIONALITY, e);
	success &= mrz_parser_parse(&s, mrz->date_of_birth, 6,
			MRZ_NUMBERS_AND_FILLER, MRZ_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_birth_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->sex, 1, MRZ_SEXES, MRZ_SEX, e);
	success &= mrz_parser_parse(&s, mrz->date_of_expiry, 6,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_expiry_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	char optional_data2[17] = {0};
	success &= mrz_parser_parse(&s, optional_data2, 16,
			MRZ_ALL, MRZ_OPTIONAL_DATA2, e);

	// Validate check sums.
	success &= mrz_parser_check(document_number_check_digit,
			mrz->document_number, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOCUMENT_NUMBER);
	success &= mrz_parser_check(date_of_birth_check_digit,
			mrz->date_of_birth, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOB);
	success &= mrz_parser_check(date_of_expiry_check_digit,
			mrz->date_of_expiry, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOE);

	return success;
}

static int mrz_parser_parse_mrvb(MRZ *mrz, const char *s) {
	if (*s != 'V') {
		mrz->error = MRZ_NO_VISA;
		return 0;
	}
	mrz->error = NULL;
	const char **e = &mrz->error;
	int success = 1;

	// First line.
	success &= mrz_parser_parse(&s, mrz->document_code, 2,
			MRZ_ALL, MRZ_DOCUMENT_CODE, e);
	success &= mrz_parser_parse(&s, mrz->issuing_state, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_ISSUING_STATE, e);
	char identifiers[32] = {0};
	success &= mrz_parser_parse(&s, identifiers, 31,
			MRZ_CHARACTERS_AND_FILLER, MRZ_IDENTIFIERS, e);
	mrz_parser_parse_identifiers(mrz, identifiers);

	// Second line.
	success &= mrz_parser_parse(&s, mrz->document_number, 9,
			MRZ_ALL, MRZ_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, document_number_check_digit, 1,
			MRZ_NUMBERS_AND_FILLER, MRZ_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->nationality, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_NATIONALITY, e);
	success &= mrz_parser_parse(&s, mrz->date_of_birth, 6,
			MRZ_NUMBERS_AND_FILLER, MRZ_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_birth_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->sex, 1, MRZ_SEXES, MRZ_SEX, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, mrz->date_of_expiry, 6,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY, e);
	success &= mrz_parser_parse(&s, date_of_expiry_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	char optional_data2[9] = {0};
	success &= mrz_parser_parse(&s, optional_data2, 8,
			MRZ_ALL, MRZ_OPTIONAL_DATA2, e);

	// Validate check sums.
	success &= mrz_parser_check(document_number_check_digit,
			mrz->document_number, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOCUMENT_NUMBER);
	success &= mrz_parser_check(date_of_birth_check_digit,
			mrz->date_of_birth, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOB);
	success &= mrz_parser_check(date_of_expiry_check_digit,
			mrz->date_of_expiry, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOE);

	return success;
}

static int mrz_parser_parse_france(MRZ *mrz, const char *s) {
	// France got its very own MRZ on ID cards:
	// https://en.wikipedia.org/wiki/National_identity_card_(France)
	mrz->error = NULL;
	const char **e = &mrz->error;
	int success = 1;

	// First line.
	success &= mrz_parser_parse(&s, mrz->document_code, 2,
			MRZ_ALL, MRZ_DOCUMENT_CODE, e);
	success &= mrz_parser_parse(&s, mrz->nationality, 3,
			MRZ_CHARACTERS_AND_FILLER, MRZ_NATIONALITY, e);
	success &= mrz_parser_parse(&s, mrz->primary_identifier, 25,
			MRZ_CHARACTERS_AND_FILLER, MRZ_IDENTIFIERS, e);
	char department_of_issuance1[4] = {0};
	success &= mrz_parser_parse(&s, department_of_issuance1, 3,
			MRZ_ALL, MRZ_DEPARTMENT_OF_ISSUANCE, e);
	char office_of_issuance[4] = {0};
	success &= mrz_parser_parse(&s, office_of_issuance, 3,
			MRZ_NUMBERS_AND_FILLER, MRZ_OFFICE_OF_ISSUANCE, e);

	// Second line.
	char year_of_issuance[3] = {0};
	success &= mrz_parser_parse(&s, year_of_issuance, 2,
			MRZ_NUMBERS, MRZ_YEAR_OF_ISSUANCE, e);
	char month_of_issuance[3] = {0};
	success &= mrz_parser_parse(&s, month_of_issuance, 2,
			MRZ_NUMBERS, MRZ_MONTH_OF_ISSUANCE, e);
	char department_of_issuance2[4] = {0};
	success &= mrz_parser_parse(&s, department_of_issuance2, 3,
			MRZ_ALL, MRZ_DEPARTMENT_OF_ISSUANCE, e);
	success &= mrz_parser_parse(&s, mrz->document_number, 5,
			MRZ_NUMBERS, MRZ_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, document_number_check_digit, 1,
			MRZ_NUMBERS, MRZ_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->secondary_identifier, 14,
			MRZ_CHARACTERS_AND_FILLER, MRZ_IDENTIFIERS, e);
	success &= mrz_parser_parse(&s, mrz->date_of_birth, 6,
			MRZ_NUMBERS_AND_FILLER, MRZ_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parser_parse(&s, date_of_birth_check_digit, 1,
			MRZ_NUMBERS, MRZ_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parser_parse(&s, mrz->sex, 1, MRZ_SEXES, MRZ_SEX, e);
	char check_digit[2] = {0};
	success &= mrz_parser_parse(&s, check_digit, 1,
			MRZ_NUMBERS, MRZ_COMBINED_CHECK_DIGIT, e);

	// Validate check sums.
	success &= mrz_parser_check(document_number_check_digit,
			year_of_issuance,
			month_of_issuance,
			department_of_issuance2,
			mrz->document_number,
			NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOCUMENT_NUMBER);
	success &= mrz_parser_check(date_of_birth_check_digit,
			mrz->date_of_birth, NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_DOB);
	success &= mrz_parser_check(check_digit,
			mrz->document_code,
			mrz->nationality,
			mrz->primary_identifier,
			department_of_issuance1,
			office_of_issuance,
			year_of_issuance,
			month_of_issuance,
			department_of_issuance2,
			mrz->document_number,
			document_number_check_digit,
			mrz->secondary_identifier,
			mrz->date_of_birth,
			date_of_birth_check_digit,
			mrz->sex,
			NULL);
	MRZ_ASSERT_CSUM(success, mrz, MRZ_CSUM_COMBINED);

	// Calculate expiry date.
	int year = atoi(year_of_issuance);
	// Add 10 years if the ID card was issued before 2014, but 15 if
	// it was issued in or after 2014. Unfortunately, only the last
	// two digits of a year are known, so we can't distiguish between
	// 1925 and 2025. Let's just say everything greater than 2050 is
	// a year of the past millenium.
	snprintf(mrz->date_of_expiry, sizeof(mrz->date_of_expiry), "%02d%2s01",
			(year + (year < 14 || year > 50 ? 10 : 15)) % 100,
			month_of_issuance);

	// Trim identifiers as we do this with other MRZs too. This cannot
	// be done before calculating the combined checksum, of course.
	mrz_parser_rtrim(mrz->primary_identifier);
	mrz_parser_rtrim(mrz->secondary_identifier);

	return success;
}

static char *mrz_parser_purify(char *dst, const char *src, size_t len) {
	const char *end = dst + len;
	while (dst < end) {
		src += strcspn(src, MRZ_ALL);
		size_t sl = strspn(src, MRZ_ALL);
		if (sl < 1) {
			break;
		}
		char *n = dst + sl;
		if (n > end) {
			return NULL;
		}
		dst = stpncpy(dst, src, sl);
		src += sl;
	}
	*dst = 0;
	return dst;
}

int mrz_parse(MRZ *mrz, const char *s) {
	if (!mrz || !s) {
		return 0;
	}
	memset(mrz, 0, sizeof(MRZ));
	char pure[91];
	if (!mrz_parser_purify(pure, s, MRZ_CAPACITY(pure))) {
		return 0;
	}
	int is_visa = *pure == 'V';
	int result = -1;
	switch (strlen(pure)) {
		case 90:
			result = mrz_parser_parse_td1(mrz, pure);
			break;
		case 72:
			result = !strncmp(s, "IDFRA", 5)
				? mrz_parser_parse_france(mrz, pure)
				: is_visa
				? mrz_parser_parse_mrvb(mrz, pure)
				: mrz_parser_parse_td2(mrz, pure);
			break;
		case 88:
			result = is_visa
				? mrz_parser_parse_mrva(mrz, pure)
				: mrz_parser_parse_td3(mrz, pure);
			break;
		default:
			return 0;
	}
	// Trim fillers.
	strtok(mrz->document_code, MRZ_FILLER);
	strtok(mrz->issuing_state, MRZ_FILLER);
	strtok(mrz->nationality, MRZ_FILLER);
	strtok(mrz->document_number, MRZ_FILLER);
	strtok(mrz->date_of_birth, MRZ_FILLER);
	strtok(mrz->date_of_expiry, MRZ_FILLER);
	return result;
}
#endif // MRZ_PARSER_IMPLEMENTATION

#endif
