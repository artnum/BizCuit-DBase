/* dbase III format reader 
 * export to mysql ? mabye
 *
 * Tuned to export winbiz software db
 *
 * With help from http://www.independent-software.com/dbase-dbf-dbt-file-format.html
 *            and https://www.dbase.com/Knowledgebase/INT/db7_file_fmt.htm
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <iconv.h>
#include <time.h>

#define TABLE_FILE_HEADER_LEN       68
#define TABLE_FILE_FIELD_DESC_LEN   48
#define TABLE_FILE_FD_TERM          0x0D

typedef struct s_dtable_header dtable_header;
typedef struct s_dfdesc dtable_fdesc;
typedef struct s_dfield dtable_field;
typedef struct s_dreco dtable_record;

struct s_dtable_header {
	uint8_t version;
	uint8_t memo_file;
	uint8_t sql_file;
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint32_t recnum;
	uint16_t hsize;
	uint16_t recsize;
	dtable_fdesc * first;
};

struct s_dfdesc {
	char name[45];
	char type;
	uint32_t memaddr;
	uint8_t length;
	uint8_t count;
	uint8_t setfield;
	
	uint32_t intable;
	
	dtable_fdesc * next;
};

struct s_dreco {
	dtable_fdesc * descriptor;
	uint32_t index;
	uint8_t deleted;
	dtable_field * first;
	dtable_record * next;
	dtable_record * previous;
};

struct s_dfield {
	dtable_fdesc * descriptor;
	
	char * text;
	long int integer;
	double number;
	uint8_t boolean;
	struct tm date;
	uint32_t memo;

	uint8_t not_init;

	dtable_field * next;
};

#define DTYPE_UNKNOWN	0x00
#define DTYPE_CHAR	0x01
#define DTYPE_DATE	0x02
#define DTYPE_FLOAT	0x03
#define DTYPE_BOOL	0x04
#define DTYPE_DATETIME	0x05
#define DTYPE_INTEGER	0x06
#define DTYPE_CURRENCY	0x07
#define DTYPE_MEMO	0x08
#define DTYPE_DOUBLE	0x09

const char MAP_TYPE[][2] = {
	{ 'Y', DTYPE_CURRENCY },
	{ 'I', DTYPE_INTEGER },
	{ 'T', DTYPE_DATETIME },
	{ 'L', DTYPE_BOOL },
	{ 'N', DTYPE_FLOAT },
	{ 'F', DTYPE_FLOAT },
	{ 'D', DTYPE_DATE },
	{ 'C', DTYPE_CHAR },
	{ 'M', DTYPE_MEMO },
	{ 'G', DTYPE_DOUBLE },
	{ '\0', '\0' }
};

const char * type_to_str (char type) {
	switch(type) {
		default:
		case DTYPE_UNKNOWN: return "unknown";
		case DTYPE_CHAR: return "char";
		case DTYPE_DATE: return "date";
		case DTYPE_FLOAT: return "float";
		case DTYPE_BOOL: return "bool";
		case DTYPE_DATETIME: return "datetime";
		case DTYPE_INTEGER: return "integer";
		case DTYPE_CURRENCY: return "currency";
		case DTYPE_MEMO: return "memo";
		case DTYPE_DOUBLE: return "double";
	}
}

dtable_fdesc * parse_fdesc (char * data, dtable_fdesc * previous) {
	dtable_fdesc * fdesc = NULL;
	iconv_t idesc;
	int i = 0;
	size_t inleft = 11;
	size_t outleft = 44;
	char * out = NULL;
	char * in = data;

	fdesc = calloc(1, sizeof(*fdesc));
	if (!fdesc) { printf("ALLOC ERROR\n"); } 
	/* iconv move pointer, use x to save */
	out = fdesc->name;	
	idesc = iconv_open("UTF-8", "ISO-8859-1");
	iconv(idesc, &in, &inleft, &out, &outleft);
	iconv_close(idesc);

	while (MAP_TYPE[i][0] != '\0') {
		if (MAP_TYPE[i][0] == *(data + 11)) {
			fdesc->type = MAP_TYPE[i][1];
			break;
		}
		i++;
	}
	
	fdesc->memaddr = 0; /* TODO */
	fdesc->length = *(data + 16);
	fdesc->count = *(data + 17);
	fdesc->setfield = *(data + 23);
	fdesc->next = NULL;
	
	if (previous) { previous->next = fdesc; }
	
	return fdesc;
}

dtable_header * parse_header (char * data) {
	dtable_header * header = NULL;

	header = malloc(sizeof(*header));
	if (!header) { return NULL; }

	header->version = 	*(data + 0) & 0x7;
	header->memo_file = 	*(data + 0) >> 7;
	header->sql_file = 	0;
	header->year =		((uint8_t)*(data + 1)) + 2000; /* doc say + 1900, but data shows + 2000 */
	header->month =		(uint8_t)*(data + 2);
	header->day =		(uint8_t)*(data + 3);

	header->recnum =	(uint32_t)(*(data + 0x07) << 24);
	header->recnum |=	(uint32_t)(*(data + 0x06) << 16);
	header->recnum |=	(uint16_t)(*(data + 0x05) << 8);
	header->recnum |=	(uint8_t)(*(data + 0x04));
	
	header->hsize =		(uint8_t)*(data + 8);
	header->hsize |=	(uint16_t)(*(data + 9) << 8);

	header->recsize =	(uint8_t)*(data + 10);
	header->recsize |=	(uint16_t)(*(data + 11) << 8);

	return header;
}

void free_header (dtable_header * header) {
	dtable_fdesc * fdesc = NULL;
	dtable_fdesc * next = NULL;

	fdesc = header->first;
	while(fdesc) {
		next = fdesc->next;
		free(fdesc);
		fdesc = next;
	}
	free(header);
}

void dump_header (dtable_header * header) {
	dtable_fdesc * fdesc = NULL;
	printf("VERSION %u %s\n", header->version, header->memo_file ? "with memo" : "without memo");
	printf("LAST UPDATE %u-%u-%u\n", header->year, header->month, header->day);
	printf("HEADER SIZE %u\n", header->hsize);
	printf("RECORD SIZE %u\n", header->recsize);
	printf("RECORD NUMBERS %u\n", header->recnum);

	fdesc = header->first;
	while(fdesc != NULL) {
		printf("\t%s TYPE %s LEN %u COUNT %u\n", fdesc->name, type_to_str(fdesc->type), fdesc->length, fdesc->count);
		fdesc = fdesc->next;
	}
}

char * load_header (FILE * fp) {
	char * header = NULL;
	rewind(fp);

	header = calloc(32, 1);
	if (!header) { printf("ALLOC ERROR\n"); }
	fread(header, 1, 32, fp);
	return header;
}

char * load_fdesc (FILE * fp, uint32_t idx) {
	char * fdesc = NULL;
	char name[12];
	fseek(fp, (long)((idx * 32) + 32), SEEK_SET);

	fdesc = calloc(32, 1);
	if (!fdesc) { printf("ALLOC ERROR\n"); }
	fread(fdesc, 1, 32, fp);

	strncpy(name, fdesc, 11);
	name[11] = '\0';

	if (fdesc[0] == 0x0D) { free(fdesc); return NULL; }
	return fdesc;
}

char * load_record (FILE * fp, uint32_t idx, dtable_header * header, char * buffer) {
	char * data = NULL;
	fseek(fp, (long)(header->hsize + (idx * header->recsize)), SEEK_SET);
	if (buffer != NULL) {
		data = buffer;
	} else {
		data = calloc(header->recsize, sizeof(*data));
	}
	if (!data) { printf("ALLOC ERROR\n"); }
	fread(data, sizeof(*data), header->recsize, fp);

	return data;
}

#define isspace(x) ((x) == 0x20 || (x) == 0x09 || (x) == 0x0A || (x) == 0x0D)

char * _get_text_field (char * data, uint32_t len) {
	char * outbuff;
	char * out = NULL;
	size_t outlen = len * 4;
	size_t inlen = len;
	char * inbuff = data;
	size_t i = 0;
	iconv_t idesc;

	outbuff = calloc(outlen, sizeof(*outbuff));
	out = outbuff;
	idesc = iconv_open("UTF-8", "ISO-8859-1");
	iconv(idesc, &inbuff, &inlen, &outbuff, &outlen);
	iconv_close(idesc);

	outbuff = out + strlen(out) - 1;
	while (outbuff > out && isspace((unsigned char)*outbuff)) { outbuff--; }
	if (isspace(*outbuff)) { *outbuff = 0x00; }
	outbuff[1] = 0x00;

	return out;
}

#define is_num(c) (((c) >= 0x30 && (c) <= 0x39) || (c) == 0x2e || (c) == 0x2d) 

double _get_float_field (char * data, uint32_t len, int * nodiv) {
	int is_dec = 0;
	int is_neg = 0;
	int num_started = 0;
	uint32_t i = 0;
	double value = 0;
	float sub = 0;
	unsigned int divider = 1;

	for (i = 0; i < len; i++) {
		if (!is_num(*(data + i))) { continue; }
		if (*(data + i) == 0x2d && !num_started) { is_neg = 1; continue; }
		num_started = 1;
		if (*(data + i) == 0x2e) { is_dec = 1; continue; }
		if (!is_dec) {
			value *= 10;
			value += (*(data + i) - 0x30);
			continue;
		}
		divider *= 10;
		sub *= 10;
		sub += (*(data + i) - 0x30);
	}
	if (sub == 0) {
		*nodiv = 1;
	} else {
		value += (sub / divider);
	}
	return is_neg ? -value : value;
}

long int _get_int_field (char * data, uint32_t len) {
	int is_neg = 0;
	int num_started = 0;
	uint32_t i = 0;
	long int value = 0;

	for (i = 0; i < len; i++) {
		if (!is_num(*(data + i))) { continue; }
		if (*(data + i) == 0x2d && !num_started) { is_neg = 1; continue; }
		if (*(data + i) == 0x2e) { break; }
		num_started = 1;
		value *= 10;
		value += (*(data + i) - 0x30);
	}
	return is_neg ? -value : value;

}

int _get_bool_field (char * data) {
	switch(*data) {
		case 'Y':
		case 'y':
		case 'T':
		case 't':
			return 1;
		case '?':
			return 2;
		default: return 0;
	}
}

#define is_int(c) (((c) >= 0x30 && (c) <= 0x39)) 
struct tm _get_date_field (char * data, uint32_t len, uint8_t *not_init) {
	uint32_t i = 0;
	struct tm date;
	date.tm_sec = 0;
	date.tm_min = 0;
	date.tm_hour = 0;
	date.tm_isdst = 1; // ch daylight saving
	date.tm_year = 0;
	date.tm_mday = 0;
	date.tm_mon = 0;

	if (isspace(*data)) { 
		mktime(&date);
		*not_init = 1;
		return date;
	}
	*not_init = 0;
	for (i = 0; i < len; i++) {
		if (!is_int(*(data + i))) {
			*not_init = 1;
			break;
		}
		if (i < 4) {
			date.tm_year *= 10;
			date.tm_year += (*(data + i) - 0x30);
		} else if (i >= 4 && i < 6) {
			date.tm_mon *= 10;
			date.tm_mon += (*(data + i) - 0x30);
		} else {
			date.tm_mday *= 10;
			date.tm_mday += (*(data + i) - 0x30);
		}
	}

	if (*not_init) {
		date.tm_year = 0;
		date.tm_mon = 0;
		date.tm_mday = 0;
	} else {
		date.tm_year -= 1900;
		date.tm_mon--; // 0 to 11
	}
	
	mktime(&date);
	return date;
}

/* memo can be either string or binary ... */
uint32_t _get_memo_field (char * data, uint32_t len, uint8_t * not_init) {
	uint32_t block = 0;
	if (len > 4) { /* string encoded */
		block = (uint32_t) _get_int_field(data, len);
		if (block == 0) {
			*not_init = 1;
		} else {
			*not_init = 0;
		}
		return block;
	}
	if (len != 4) { *not_init = 1; return 0; }
	/* binary encoded */
	block = *(data);
	block |= *(data + 1) << 8;
	block |= *(data + 2) << 16;
	block |= *(data + 3) << 24;
	return block;
}

struct tm _get_datetime_field (char * data, uint32_t len) {
}

/* dbase number can be int or float, this allow, by looking at the data to
 * distinguish them
 */
void preflight_record (char * data, dtable_header * header) {
	int nodiv = 0;
	uint32_t pos = 1;
	dtable_fdesc * hcurrent = header->first;

	while (hcurrent) {
		if (hcurrent->type != DTYPE_FLOAT) { 
			pos += hcurrent->length; 
			hcurrent = hcurrent->next;
			continue;
		}
		_get_float_field(data + pos, hcurrent->length, &nodiv);
		if (nodiv) {
			hcurrent->intable++;
		}
		hcurrent = hcurrent->next;
	}
}

dtable_record * parse_record (char * data, dtable_header * header) {
	dtable_fdesc * hcurrent = header->first;
	dtable_record * record = NULL;
	dtable_field * fcurrent = NULL;
	dtable_field * fprev = NULL;
	uint32_t pos = 1;
	int nodiv = 0;

	record = calloc(1, sizeof(*record));

	record->deleted = *data == '*' ? 1 : 0;

	while(hcurrent) {
		nodiv = 0;
		fcurrent = calloc(1, sizeof(*fcurrent));
		fcurrent->next = NULL;
		if (fprev) { fprev->next = fcurrent; }
		fprev = fcurrent;
		if (!record->first) { record->first = fcurrent; }
		fcurrent->descriptor = hcurrent;
		fcurrent->not_init = 0;
		switch(hcurrent->type) {
			case DTYPE_MEMO:
				fcurrent->memo = _get_memo_field(data + pos, hcurrent->length, &(fcurrent->not_init));
				break;
			case DTYPE_DATE:
				fcurrent->date = _get_date_field(data + pos, hcurrent->length, &(fcurrent->not_init));
				break;
			case DTYPE_INTEGER:
				fcurrent->integer = _get_int_field(data + pos, hcurrent->length);
				break;
			case DTYPE_FLOAT:
				if (hcurrent->intable == header->recnum) {
					fcurrent->integer = _get_int_field(data + pos, hcurrent->length);
					hcurrent->type = DTYPE_INTEGER;
					break;
				}
				fcurrent->number = _get_float_field(data + pos, hcurrent->length, &nodiv);
				if (nodiv) {
					hcurrent->intable++;
				}
				break;
			case DTYPE_CHAR:
				fcurrent->text = _get_text_field(data + pos, hcurrent->length);
				break;	
			case DTYPE_BOOL:
				fcurrent->boolean = _get_bool_field(data + pos);
				if (fcurrent->boolean > 1) { 
					fcurrent->boolean = 0;
				       	fcurrent->not_init = 1; 
				}
				break;
		}
		pos += hcurrent->length;
		hcurrent = hcurrent->next;
	}
	return record;
}

void free_record (dtable_record * record) {
	dtable_field * c = NULL;
	dtable_field * tmp = NULL;

	c = record->first;
	while (c) {
		tmp = c->next;
		if (c->text) { free(c->text); }
		free(c);
		c = tmp;
	}
	free(record);
}

int main (int argc, char ** argv) {
	FILE *fp = NULL;
	dtable_header * header = NULL;
	dtable_fdesc * fdesc = NULL;
	dtable_record * record = NULL;
	dtable_field * field = NULL;
	char * buffer = NULL;
	uint32_t i = 0;
	int j = 0;
	char datebuff[11];

	if (argc < 2) { return 0; }

	fp = fopen(argv[1], "r");
	if (fp == NULL) { return 0; }

	buffer = load_header(fp);
	header = parse_header(buffer);
	free(buffer);

	while ((buffer = load_fdesc(fp, i++)) != NULL) {
		fdesc = parse_fdesc(buffer, fdesc);
		if (!header->first) { header->first = fdesc; }
		free(buffer);
	}
	dump_header(header);

	buffer = NULL;
	for (j = 0; j < 2; j++) {	
		for (i = 0; i < header->recnum; i++) {
			
			buffer = load_record(fp, i, header, buffer);
			if (j == 0) {
				preflight_record(buffer, header);
				continue;
			}
			record = parse_record(buffer, header);
			record->index = i;

			field = record->first;

			printf("%cRECORD <%d>\n", record->deleted ? '-' : '+', record->index);
			while (field) {
				printf("\t%s:", field->descriptor->name);
				switch (field->descriptor->type) {
					case DTYPE_DATE:
						if (field->not_init) {
							printf(" unset\n");
							break;
						}
						strftime(datebuff, 11, "%Y-%m-%d", &(field->date));
						printf(" %s\n", datebuff);
						break;
					case DTYPE_INTEGER:
						printf(" %d\n", field->integer);
						break;
					case DTYPE_FLOAT:
						printf(" %.4f\n", field->number);
						break;
					case DTYPE_MEMO:
						if (field->not_init) {
							printf(" unset\n");
							break;
						}
						printf(" @block %d\n", field->memo);
						break;
					case DTYPE_CHAR:
						printf(" \"%s\"\n",  field->text);
						break;
					case DTYPE_BOOL:
						if (field->not_init) {
							printf(" unset\n");
							break;
						}
						printf(" %s\n",  field->boolean ? "true" : "false");
						break;
					default: 
						printf(" ?\n");
						break;
				}
				field = field->next;
			}
			free_record(record);
		}
	}
	free(buffer);
	fdesc = header->first;
	while (fdesc) {
		if (fdesc->intable == header->recsize) {
			printf("%s is INTABLE\n", fdesc->name);
		}
		fdesc = fdesc->next;
	}
	
	free_header(header);

	fclose(fp);
}
