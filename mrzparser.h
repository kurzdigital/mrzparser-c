#ifndef __mrzparser_h_
#define __mrzparser_h_

#define MRZ_ERROR_DOCUMENT_CODE 1
#define MRZ_ERROR_ISSUING_STATE 2
#define MRZ_ERROR_DOCUMENT_NUMBER 3
#define MRZ_ERROR_DOCUMENT_NUMBER_CHECK_DIGIT 4
#define MRZ_ERROR_OPTIONAL_DATA1 5
#define MRZ_ERROR_DATE_OF_BIRTH 6
#define MRZ_ERROR_DATE_OF_BIRTH_CHECK_DIGIT 7
#define MRZ_ERROR_SEX 8
#define MRZ_ERROR_DATE_OF_EXPIRY 9
#define MRZ_ERROR_DATE_OF_EXPIRY_CHECK_DIGIT 10
#define MRZ_ERROR_NATIONALITY 11
#define MRZ_ERROR_OPTIONAL_DATA2 12
#define MRZ_ERROR_PERSONAL_NUMBER 13
#define MRZ_ERROR_PERSONAL_NUMBER_CHECK_DIGIT 14
#define MRZ_ERROR_COMBINED_CHECK_DIGIT 15
#define MRZ_ERROR_VISA 16
#define MRZ_ERROR_NO_VISA 17
#define MRZ_ERROR_NOT_FRANCE 18
#define MRZ_ERROR_DEPARTMENT_OF_ISSUANCE 19
#define MRZ_ERROR_OFFICE_OF_ISSUANCE 20
#define MRZ_ERROR_YEAR_OF_ISSUANCE 21
#define MRZ_ERROR_MONTH_OF_ISSUANCE 22
#define MRZ_ERROR_IDENTIFIERS 23
#define MRZ_ERROR_CSUM_DOCUMENT_NUMBER 24
#define MRZ_ERROR_CSUM_DOB 25
#define MRZ_ERROR_CSUM_DOE 26
#define MRZ_ERROR_CSUM_PERSONAL_NUMBER 27
#define MRZ_ERROR_CSUM_COMBINED 28
#define MRZ_ERROR_SWISS_BLANK_NUMBER 29
#define MRZ_ERROR_SWISS_LANGUAGE 30
#define MRZ_ERROR_SWISS_VERSION 31
#define MRZ_ERROR_SWISS_FILLER 32
#define MRZ_MAX_ERRORS MRZ_ERROR_SWISS_FILLER

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
	char optional_data1[16];
	char optional_data2[17];
	char blank_number[7];
	char language[4];
	int errors[MRZ_MAX_ERRORS];
};
typedef struct MRZ MRZ;

int parse_mrz(struct MRZ *, const char *);

const char *mrz_error_string(int code) {
	switch (code) {
	default: return "unknown";
	case MRZ_ERROR_DOCUMENT_CODE: return "document code";
	case MRZ_ERROR_ISSUING_STATE: return "issuing state";
	case MRZ_ERROR_DOCUMENT_NUMBER: return "document number";
	case MRZ_ERROR_DOCUMENT_NUMBER_CHECK_DIGIT: return "document number check digit";
	case MRZ_ERROR_OPTIONAL_DATA1: return "optional data1";
	case MRZ_ERROR_DATE_OF_BIRTH: return "date of birth";
	case MRZ_ERROR_DATE_OF_BIRTH_CHECK_DIGIT: return "date of birth check digit";
	case MRZ_ERROR_SEX: return "sex";
	case MRZ_ERROR_DATE_OF_EXPIRY: return "date of expiry";
	case MRZ_ERROR_DATE_OF_EXPIRY_CHECK_DIGIT: return "date of expiry check digit";
	case MRZ_ERROR_NATIONALITY: return "nationality";
	case MRZ_ERROR_OPTIONAL_DATA2: return "optional data2";
	case MRZ_ERROR_PERSONAL_NUMBER: return "personal number";
	case MRZ_ERROR_PERSONAL_NUMBER_CHECK_DIGIT: return "personal number check digit";
	case MRZ_ERROR_COMBINED_CHECK_DIGIT: return "combined check digit";
	case MRZ_ERROR_VISA: return "visa";
	case MRZ_ERROR_NO_VISA: return "no visa";
	case MRZ_ERROR_NOT_FRANCE: return "not france";
	case MRZ_ERROR_DEPARTMENT_OF_ISSUANCE: return "department of issuance";
	case MRZ_ERROR_OFFICE_OF_ISSUANCE: return "office of issuance";
	case MRZ_ERROR_YEAR_OF_ISSUANCE: return "year of issuance";
	case MRZ_ERROR_MONTH_OF_ISSUANCE: return "month of issuance";
	case MRZ_ERROR_IDENTIFIERS: return "names";
	case MRZ_ERROR_CSUM_DOCUMENT_NUMBER: return "checksum for document number";
	case MRZ_ERROR_CSUM_DOB: return "checksum for date of birth";
	case MRZ_ERROR_CSUM_DOE: return "checksum for date of expiry";
	case MRZ_ERROR_CSUM_PERSONAL_NUMBER: return "checksum for personal number";
	case MRZ_ERROR_CSUM_COMBINED: return "combined checksum";
	case MRZ_ERROR_SWISS_BLANK_NUMBER: return "blank number";
	case MRZ_ERROR_SWISS_LANGUAGE: return "language code";
	case MRZ_ERROR_SWISS_VERSION: return "version";
	case MRZ_ERROR_SWISS_FILLER: return "filler characters";
	}
}

#ifdef MRZ_PARSER_IMPLEMENTATION
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MRZ_CHARACTERS "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define MRZ_NUMBERS "0123456789"
#define MRZ_FILLER "<"
#define MRZ_SEXES "MFX" MRZ_FILLER
#define MRZ_ALL MRZ_CHARACTERS MRZ_NUMBERS MRZ_FILLER
#define MRZ_CHARACTERS_AND_FILLER MRZ_CHARACTERS MRZ_FILLER
#define MRZ_NUMBERS_AND_FILLER MRZ_NUMBERS MRZ_FILLER
#define MRZ_CAPACITY(s) (sizeof(s) - 1)
#define MRZ_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MRZ_FILLER_SEPARATOR "<<"
#define MRZ_WHITE_SPACE " "

static void mrz_add_error(int *error, int code) {
	for (int *end = error + MRZ_MAX_ERRORS; error < end; ++error) {
		if (!*error) {
			*error = code;
			break;
		}
	}
}

static int mrz_assert_checksum(int result, MRZ *mrz, int code) {
	if (!result) {
		mrz_add_error(mrz->errors, code);
	}
	return result;
}

static int mrz_check_digit_weights[] = {7, 3, 1};
static int mrz_check(const char *digit, ...) {
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
			sum += c * mrz_check_digit_weights[i % 3];
		}
	}
	sum %= 10;
	va_end(ap);
	return sum == (*digit == '<' ? 0 : *digit - 48);
}

static int mrz_check_and_expand_extended_document_number(
		const char *digit,
		char *dn,
		size_t dn_cap,
		const char *ext) {
	if (*digit == '<') {
		const char *p = strchr(ext, '<');
		size_t len = p - ext;
		if (p && len > 1) {
			char d[2] = {ext[--len], 0};
			// Unfortunately, Note j in ICAO 9303p5 doesn't specify
			// if the `<` shall be part of the checksum calculation
			// or not. This means some issuers include the `<` and
			// other don't so we need to accept both variants.
			char with[91] = {0};
			size_t cap_w = MRZ_CAPACITY(with);
			strncat(with, dn, cap_w);
			strncat(with, MRZ_FILLER, cap_w - strlen(with));
			if (cap_w - strlen(with) < len) {
				return 0;
			}
			strncat(with, ext, len);
			char without[91] = {0};
			size_t cap_wo = MRZ_CAPACITY(without);
			strncat(without, dn, cap_wo);
			if (cap_wo - strlen(without) < len) {
				return 0;
			}
			strncat(without, ext, len);
			if (mrz_check(d, with, NULL) ||
					mrz_check(d, without, NULL)) {
				// Add extension to document number.
				strncpy(dn, without, dn_cap);
				return 1;
			}
		}
	}
	return mrz_check(digit, dn, NULL);
}

static void mrz_trim_fillers(char *s) {
	// Trim start.
	size_t skip = strspn(s, MRZ_FILLER);
	if (skip > 0) {
		char *dst = s;
		for (char *src = s + skip; *src; ) {
			*dst++ = *src++;
		}
		*dst = 0;
	}
	// Trim end.
	{
		char *r = s + strlen(s);
		for (; r > s && *(r - 1) == *MRZ_FILLER; --r);
		*r = 0;
	}
}

static void mrz_replace_fillers(char *s) {
	char *p = s;
	while ((p = strchr(p, *MRZ_FILLER))) {
		*p = *MRZ_WHITE_SPACE;
	}
}

static void mrz_parse_identifiers(MRZ *mrz, const char *identifiers) {
	const char *p = strstr(identifiers, MRZ_FILLER_SEPARATOR);
	size_t cap = MRZ_CAPACITY(mrz->primary_identifier);
	size_t len = p - identifiers;
	if (p && len < cap) {
		cap = len;
		strncpy(mrz->secondary_identifier, p + 2,
				MRZ_CAPACITY(mrz->secondary_identifier));
		mrz_trim_fillers(mrz->secondary_identifier);
		mrz_replace_fillers(mrz->secondary_identifier);
	}
	strncpy(mrz->primary_identifier, identifiers, cap);
	mrz_trim_fillers(mrz->primary_identifier);
	mrz_replace_fillers(mrz->primary_identifier);
}

static int mrz_parse_component(const char **src, size_t size, char *field,
		size_t len, const char *allowed,
		int error_code, int *errors) {
	if (!**src || size < len) {
		return 0;
	}
	int invalid = strspn(*src, allowed) < len;
	if (invalid) {
		mrz_add_error(errors, error_code);
		// Check premature end of input.
		if (strlen(*src) < len) {
			// Move to the end of the string to make sure all
			// subsequent calls to mrz_parse_component() will fail, too.
			for (; **src; ++*src);
			return 0;
		}
		// Otherwise take malformed component and keep parsing.
	}
	strncpy(field, *src, len);
	*src += len;
	return invalid ^ 1;
}

static int mrz_parse_td1(MRZ *mrz, const char *s) {
	int *e = mrz->errors;
	int success = 1;

	// First line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_code), mrz->document_code,
			2, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_CODE, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->issuing_state), mrz->issuing_state,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_ISSUING_STATE, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_number), mrz->document_number,
			9, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(document_number_check_digit), document_number_check_digit,
			1, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->optional_data1), mrz->optional_data1,
			15, MRZ_ALL,
			MRZ_ERROR_OPTIONAL_DATA1, e);

	// Second line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_birth), mrz->date_of_birth,
			6, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_birth_check_digit), date_of_birth_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->sex), mrz->sex,
			1, MRZ_SEXES,
			MRZ_ERROR_SEX, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_expiry), mrz->date_of_expiry,
			6, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_expiry_check_digit), date_of_expiry_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->nationality), mrz->nationality,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_NATIONALITY, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->optional_data2), mrz->optional_data2,
			11, MRZ_ALL,
			MRZ_ERROR_OPTIONAL_DATA2, e);
	char check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(check_digit), check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_COMBINED_CHECK_DIGIT, e);

	// Third line.
	char identifiers[31] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(identifiers), identifiers,
			30, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_IDENTIFIERS, e);
	mrz_parse_identifiers(mrz, identifiers);

	// Validate check sums.
	success &= mrz_assert_checksum(
			mrz_check(check_digit,
					mrz->document_number,
					document_number_check_digit,
					mrz->optional_data1,
					mrz->date_of_birth,
					date_of_birth_check_digit,
					mrz->date_of_expiry,
					date_of_expiry_check_digit,
					mrz->optional_data2,
					NULL),
			mrz, MRZ_ERROR_CSUM_COMBINED);
	success &= mrz_assert_checksum(
			mrz_check_and_expand_extended_document_number(
					document_number_check_digit,
					mrz->document_number,
					MRZ_CAPACITY(mrz->document_number),
					mrz->optional_data1),
			mrz, MRZ_ERROR_CSUM_DOCUMENT_NUMBER);
	success &= mrz_assert_checksum(
			mrz_check(date_of_birth_check_digit, mrz->date_of_birth, NULL),
			mrz, MRZ_ERROR_CSUM_DOB);
	success &= mrz_assert_checksum(
			mrz_check(date_of_expiry_check_digit, mrz->date_of_expiry, NULL),
			mrz, MRZ_ERROR_CSUM_DOE);

	return success;
}

static int mrz_parse_td2(MRZ *mrz, const char *s) {
	int *e = mrz->errors;
	if (*s == 'V') {
		mrz_add_error(e, MRZ_ERROR_VISA);
		return 0;
	}
	int success = 1;

	// First line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_code), mrz->document_code,
			2, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_CODE, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->issuing_state), mrz->issuing_state,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_ISSUING_STATE, e);
	char identifiers[32] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(identifiers), identifiers,
			31, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_IDENTIFIERS, e);
	mrz_parse_identifiers(mrz, identifiers);

	// Second line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_number), mrz->document_number,
			9, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(document_number_check_digit), document_number_check_digit,
			1, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->nationality), mrz->nationality,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_NATIONALITY, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_birth), mrz->date_of_birth,
			6, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_birth_check_digit), date_of_birth_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->sex), mrz->sex,
			1, MRZ_SEXES,
			MRZ_ERROR_SEX, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_expiry), mrz->date_of_expiry,
			6, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_expiry_check_digit), date_of_expiry_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->optional_data2), mrz->optional_data2,
			7, MRZ_ALL,
			MRZ_ERROR_OPTIONAL_DATA2, e);
	char check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(check_digit), check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_COMBINED_CHECK_DIGIT, e);

	// Validate check sums.
	success &= mrz_assert_checksum(
			mrz_check(check_digit,
					mrz->document_number,
					document_number_check_digit,
					mrz->date_of_birth,
					date_of_birth_check_digit,
					mrz->date_of_expiry,
					date_of_expiry_check_digit,
					mrz->optional_data2,
					NULL),
			mrz, MRZ_ERROR_CSUM_COMBINED);
	success &= mrz_assert_checksum(
			mrz_check_and_expand_extended_document_number(
					document_number_check_digit,
					mrz->document_number,
					MRZ_CAPACITY(mrz->document_number),
					mrz->optional_data2),
			mrz, MRZ_ERROR_CSUM_DOCUMENT_NUMBER);
	success &= mrz_assert_checksum(
			mrz_check(date_of_birth_check_digit, mrz->date_of_birth, NULL),
			mrz, MRZ_ERROR_CSUM_DOB);
	success &= mrz_assert_checksum(
			mrz_check(date_of_expiry_check_digit, mrz->date_of_expiry, NULL),
			mrz, MRZ_ERROR_CSUM_DOE);

	return success;
}

static int mrz_parse_td3(MRZ *mrz, const char *s) {
	int *e = mrz->errors;
	if (*s == 'V') {
		mrz_add_error(e, MRZ_ERROR_VISA);
		return 0;
	}
	int success = 1;

	// First line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_code), mrz->document_code,
			2, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_CODE, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->issuing_state), mrz->issuing_state,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_ISSUING_STATE, e);
	char identifiers[40] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(identifiers), identifiers,
			39, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_IDENTIFIERS, e);
	mrz_parse_identifiers(mrz, identifiers);

	// Second line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_number), mrz->document_number,
			9, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(document_number_check_digit), document_number_check_digit,
			1, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->nationality), mrz->nationality,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_NATIONALITY, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_birth), mrz->date_of_birth,
			6, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_birth_check_digit), date_of_birth_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->sex), mrz->sex,
			1, MRZ_SEXES,
			MRZ_ERROR_SEX, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_expiry), mrz->date_of_expiry,
			6, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_expiry_check_digit), date_of_expiry_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	char personal_number[15] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(personal_number), personal_number,
			14, MRZ_ALL,
			MRZ_ERROR_PERSONAL_NUMBER, e);
	char personal_number_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(personal_number_check_digit), personal_number_check_digit,
			1, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_PERSONAL_NUMBER_CHECK_DIGIT, e);
	char check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(check_digit), check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_COMBINED_CHECK_DIGIT, e);

	// Validate check sums.
	success &= mrz_assert_checksum(
			mrz_check(check_digit,
				mrz->document_number,
				document_number_check_digit,
				mrz->date_of_birth,
				date_of_birth_check_digit,
				mrz->date_of_expiry,
				date_of_expiry_check_digit,
				personal_number,
				personal_number_check_digit,
				NULL),
			mrz, MRZ_ERROR_CSUM_COMBINED);
	success &= mrz_assert_checksum(
			mrz_check(document_number_check_digit, mrz->document_number, NULL),
			mrz, MRZ_ERROR_CSUM_DOCUMENT_NUMBER);
	success &= mrz_assert_checksum(
			mrz_check(date_of_birth_check_digit, mrz->date_of_birth, NULL),
			mrz, MRZ_ERROR_CSUM_DOB);
	success &= mrz_assert_checksum(
			mrz_check(date_of_expiry_check_digit, mrz->date_of_expiry, NULL),
			mrz, MRZ_ERROR_CSUM_DOE);
	success &= mrz_assert_checksum(
			mrz_check(personal_number_check_digit, personal_number, NULL),
			mrz, MRZ_ERROR_CSUM_PERSONAL_NUMBER);

	return success;
}

static int mrz_parse_mrva(MRZ *mrz, const char *s) {
	int *e = mrz->errors;
	if (*s != 'V') {
		mrz_add_error(e, MRZ_ERROR_NO_VISA);
		return 0;
	}
	int success = 1;

	// First line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_code), mrz->document_code,
			2, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_CODE, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->issuing_state), mrz->issuing_state,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_ISSUING_STATE, e);
	char identifiers[40] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(identifiers), identifiers,
			39, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_IDENTIFIERS, e);
	mrz_parse_identifiers(mrz, identifiers);

	// Second line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_number), mrz->document_number,
			9, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(document_number_check_digit), document_number_check_digit,
			1, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->nationality), mrz->nationality,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_NATIONALITY, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_birth), mrz->date_of_birth,
			6, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_birth_check_digit), date_of_birth_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->sex), mrz->sex,
			1, MRZ_SEXES,
			MRZ_ERROR_SEX, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_expiry), mrz->date_of_expiry,
			6, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_expiry_check_digit), date_of_expiry_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->optional_data2), mrz->optional_data2,
			16, MRZ_ALL,
			MRZ_ERROR_OPTIONAL_DATA2, e);

	// Validate check sums.
	success &= mrz_assert_checksum(
			mrz_check(document_number_check_digit, mrz->document_number, NULL),
			mrz, MRZ_ERROR_CSUM_DOCUMENT_NUMBER);
	success &= mrz_assert_checksum(
			mrz_check(date_of_birth_check_digit, mrz->date_of_birth, NULL),
			mrz, MRZ_ERROR_CSUM_DOB);
	success &= mrz_assert_checksum(
			mrz_check(date_of_expiry_check_digit, mrz->date_of_expiry, NULL),
			mrz, MRZ_ERROR_CSUM_DOE);

	return success;
}

static int mrz_parse_mrvb(MRZ *mrz, const char *s) {
	int *e = mrz->errors;
	if (*s != 'V') {
		mrz_add_error(e, MRZ_ERROR_NO_VISA);
		return 0;
	}
	int success = 1;

	// First line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_code), mrz->document_code,
			2, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_CODE, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->issuing_state), mrz->issuing_state,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_ISSUING_STATE, e);
	char identifiers[32] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(identifiers), identifiers,
			31, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_IDENTIFIERS, e);
	mrz_parse_identifiers(mrz, identifiers);

	// Second line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_number), mrz->document_number,
			9, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(document_number_check_digit), document_number_check_digit,
			1, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->nationality), mrz->nationality,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_NATIONALITY, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_birth), mrz->date_of_birth,
			6, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_birth_check_digit), date_of_birth_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->sex), mrz->sex,
			1, MRZ_SEXES,
			MRZ_ERROR_SEX, e);
	char date_of_expiry_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_expiry), mrz->date_of_expiry,
			6, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_expiry_check_digit), date_of_expiry_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_EXPIRY_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->optional_data2), mrz->optional_data2,
			8, MRZ_ALL,
			MRZ_ERROR_OPTIONAL_DATA2, e);

	// Validate check sums.
	success &= mrz_assert_checksum(
			mrz_check(document_number_check_digit, mrz->document_number, NULL),
			mrz, MRZ_ERROR_CSUM_DOCUMENT_NUMBER);
	success &= mrz_assert_checksum(
			mrz_check(date_of_birth_check_digit, mrz->date_of_birth, NULL),
			mrz, MRZ_ERROR_CSUM_DOB);
	success &= mrz_assert_checksum(
			mrz_check(date_of_expiry_check_digit, mrz->date_of_expiry, NULL),
			mrz, MRZ_ERROR_CSUM_DOE);

	return success;
}

static int mrz_parse_france(MRZ *mrz, const char *s) {
	// France got its very own MRZ on ID cards:
	// https://en.wikipedia.org/wiki/National_identity_card_(France)
	int *e = mrz->errors;
	int success = 1;

	// First line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_code), mrz->document_code,
			2, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_CODE, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->nationality), mrz->nationality,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_NATIONALITY, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->primary_identifier), mrz->primary_identifier,
			25, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_IDENTIFIERS, e);
	char department_of_issuance1[4] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(department_of_issuance1), department_of_issuance1,
			3, MRZ_ALL,
			MRZ_ERROR_DEPARTMENT_OF_ISSUANCE, e);
	char office_of_issuance[4] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(office_of_issuance), office_of_issuance,
			3, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_OFFICE_OF_ISSUANCE, e);

	// Second line.
	char year_of_issuance[3] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(year_of_issuance), year_of_issuance,
			2, MRZ_NUMBERS,
			MRZ_ERROR_YEAR_OF_ISSUANCE, e);
	char month_of_issuance[3] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(month_of_issuance), month_of_issuance,
			2, MRZ_NUMBERS,
			MRZ_ERROR_MONTH_OF_ISSUANCE, e);
	char department_of_issuance2[4] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(department_of_issuance2), department_of_issuance2,
			3, MRZ_ALL,
			MRZ_ERROR_DEPARTMENT_OF_ISSUANCE, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_number), mrz->document_number,
			5, MRZ_NUMBERS,
			MRZ_ERROR_DOCUMENT_NUMBER, e);
	char document_number_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(document_number_check_digit), document_number_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DOCUMENT_NUMBER_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->secondary_identifier), mrz->secondary_identifier,
			14, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_IDENTIFIERS, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_birth), mrz->date_of_birth,
			6, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DATE_OF_BIRTH, e);
	char date_of_birth_check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(date_of_birth_check_digit), date_of_birth_check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_DATE_OF_BIRTH_CHECK_DIGIT, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->sex), mrz->sex,
			1, MRZ_SEXES,
			MRZ_ERROR_SEX, e);
	char check_digit[2] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(check_digit), check_digit,
			1, MRZ_NUMBERS,
			MRZ_ERROR_COMBINED_CHECK_DIGIT, e);

	// Validate check sums.
	success &= mrz_assert_checksum(
			mrz_check(check_digit,
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
					NULL),
			mrz, MRZ_ERROR_CSUM_COMBINED);
	success &= mrz_assert_checksum(
			mrz_check(document_number_check_digit,
					year_of_issuance,
					month_of_issuance,
					department_of_issuance2,
					mrz->document_number,
					NULL),
			mrz, MRZ_ERROR_CSUM_DOCUMENT_NUMBER);
	success &= mrz_assert_checksum(
			mrz_check(date_of_birth_check_digit, mrz->date_of_birth, NULL),
			mrz, MRZ_ERROR_CSUM_DOB);

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
	mrz_trim_fillers(mrz->primary_identifier);
	mrz_trim_fillers(mrz->secondary_identifier);

	return success;
}

static int mrz_parse_dl_swiss(MRZ *mrz, const char *s) {
	// Switzerland has something like an MRZ on its driver licenses.
	// See "doc/swiss_fak.pdf".
	int *e = mrz->errors;
	int success = 1;

	// First line, which is much shorter and contains just meta data.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->blank_number), mrz->blank_number,
			6, MRZ_ALL,
			MRZ_ERROR_SWISS_BLANK_NUMBER, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->language), mrz->language,
			3, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_SWISS_LANGUAGE, e);
	if (!strchr("DFIR", *mrz->language)) {
		mrz_add_error(e, MRZ_ERROR_SWISS_LANGUAGE);
		success = 0;
	}

	// Second line.
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_code), mrz->document_code,
			2, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_CODE, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->issuing_state), mrz->issuing_state,
			3, "CHE",
			MRZ_ERROR_ISSUING_STATE, e);
	if (strncmp("CHE", mrz->issuing_state, 3)) {
		mrz_add_error(e, MRZ_ERROR_ISSUING_STATE);
		success = 0;
	}

	// Calculate length of document number. Unfortunately, there are
	// (at least) two versions with a different length.
	const char *p = strstr(s, MRZ_FILLER_SEPARATOR);
	if (!p) {
		mrz_add_error(e, MRZ_ERROR_DOCUMENT_NUMBER);
		return 0;
	}
	int dnlen = p - s;
	int vlen = 3;
	int remaining;
	if (dnlen == 12) {
		dnlen = 9;
		remaining = 5;
	} else if (dnlen == 15) {
		dnlen = 12;
		remaining = 2;
	} else if (dnlen == 16) {
		dnlen = 12;
		remaining = 2;
		vlen = 4;
	} else {
		mrz_add_error(e, MRZ_ERROR_DOCUMENT_NUMBER);
		return 0;
	}

	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->document_number), mrz->document_number,
			dnlen, MRZ_ALL,
			MRZ_ERROR_DOCUMENT_NUMBER, e);
	char fillers[7];
	char version[5];
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(version), version,
			vlen, MRZ_NUMBERS,
			MRZ_ERROR_SWISS_VERSION, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(fillers), fillers,
			2, MRZ_FILLER,
			MRZ_ERROR_SWISS_FILLER, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(mrz->date_of_birth), mrz->date_of_birth,
			6, MRZ_NUMBERS_AND_FILLER,
			MRZ_ERROR_DATE_OF_BIRTH, e);
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(fillers), fillers,
			remaining, MRZ_FILLER,
			MRZ_ERROR_SWISS_FILLER, e);

	// Third line.
	int len = dnlen + vlen + 2 + 6 + remaining;
	char identifiers[32] = {0};
	success &= mrz_parse_component(&s,
			MRZ_CAPACITY(identifiers), identifiers,
			len, MRZ_CHARACTERS_AND_FILLER,
			MRZ_ERROR_IDENTIFIERS, e);
	mrz_parse_identifiers(mrz, identifiers);

	return success;
}

static char *mrz_purify(char *dst, const char *src, size_t len) {
	const char *end = dst + len;
	while (dst <= end) {
		src += strcspn(src, MRZ_ALL);
		size_t sl = strspn(src, MRZ_ALL);
		if (sl < 1) {
			break;
		}
		if (dst + sl > end) {
			return NULL;
		}
		strncpy(dst, src, sl);
		dst += sl;
		src += sl;
	}
	*dst = 0;
	return dst;
}

int parse_mrz(MRZ *mrz, const char *s) {
	if (!mrz || !s) {
		return 0;
	}
	memset(mrz, 0, sizeof(MRZ));
	char pure[91];
	if (!mrz_purify(pure, s, MRZ_CAPACITY(pure))) {
		return 0;
	}
	int is_visa = *pure == 'V';
	int result = -1;
	switch (strlen(pure)) {
		case 90:
			result = mrz_parse_td1(mrz, pure);
			break;
		case 69:
		case 71:
			result = mrz_parse_dl_swiss(mrz, pure);
			break;
		case 72:
			result = !strncmp(pure, "IDFRA", 5)
				? mrz_parse_france(mrz, pure)
				: is_visa
				? mrz_parse_mrvb(mrz, pure)
				: mrz_parse_td2(mrz, pure);
			break;
		case 88:
			result = is_visa
				? mrz_parse_mrva(mrz, pure)
				: mrz_parse_td3(mrz, pure);
			break;
		default:
			return 0;
	}
	// Trim fillers.
	mrz_trim_fillers(mrz->document_code);
	mrz_trim_fillers(mrz->issuing_state);
	mrz_trim_fillers(mrz->nationality);
	mrz_trim_fillers(mrz->document_number);
	mrz_trim_fillers(mrz->date_of_birth);
	mrz_trim_fillers(mrz->date_of_expiry);
	mrz_trim_fillers(mrz->optional_data1);
	mrz_trim_fillers(mrz->optional_data2);
	mrz_trim_fillers(mrz->blank_number);
	mrz_trim_fillers(mrz->language);
	// Replace fillers with white space.
	mrz_replace_fillers(mrz->document_code);
	mrz_replace_fillers(mrz->issuing_state);
	mrz_replace_fillers(mrz->nationality);
	mrz_replace_fillers(mrz->document_number);
	mrz_replace_fillers(mrz->date_of_birth);
	mrz_replace_fillers(mrz->sex);
	mrz_replace_fillers(mrz->date_of_expiry);
	return result;
}
#endif // MRZ_PARSER_IMPLEMENTATION

#endif
