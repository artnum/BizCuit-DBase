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

#include "dbase.h"

#define TABLE_FILE_HEADER_LEN       68
#define TABLE_FILE_FIELD_DESC_LEN   48
#define TABLE_FILE_FD_TERM          0x0D

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

dtable_fdesc * parse_fdesc (char * data, dtable_fdesc * previous, iconv_t idesc) {
	dtable_fdesc * fdesc = NULL;
	int i = 0;
	size_t inleft = 11;
	size_t outleft = 44;
	char * out = NULL;
	char * in = data;

	fdesc = calloc(1, sizeof(*fdesc));
	if (!fdesc) { return NULL; }

	
	out = fdesc->name;	
	iconv(idesc, &in, &inleft, &out, &outleft);

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

	header = calloc(1, sizeof(*header));
	if (!header) { return NULL; }

	header->version = 	*(data + 0) & 0x7;
	header->memo_file = 	*(data + 0) >> 7;
	header->sql_file = 	0;
	header->year =		((uint8_t)*(data + 1)) + 2000; /* doc say + 1900, but data shows + 2000 */
	header->month =		(uint8_t)*(data + 2);
	header->day =		(uint8_t)*(data + 3);

	header->recnum =	(uint32_t)((*(data + 0x07) & 0xFF) << 24);
	header->recnum |=	(uint32_t)((*(data + 0x06) & 0xFF) << 16);
	header->recnum |=	(uint32_t)((*(data + 0x05) & 0xFF) << 8);
	header->recnum |=	(uint32_t)(*(data + 0x04)) & 0xFF;
	
	header->hsize =		(uint16_t)(*(data + 8)) & 0xFF;
	header->hsize |=	(uint16_t)((*(data + 9) & 0xFF) << 8);

	header->recsize =	(uint16_t)(*(data + 10)) & 0xFF;
	header->recsize |=	(uint16_t)((*(data + 11) & 0xFF) << 8);

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
	if (header->memo) {
		fclose(header->memo);
	}
	if (header->idesc != (iconv_t)-1) { iconv_close(header->idesc); }
	free(header);
}

void dump_header (dtable_header * header) {
	dtable_fdesc * fdesc = NULL;
	char type = ' ';
	printf("VERSION %u %s\n", header->version, header->memo_file ? "with memo" : "without memo");
	printf("LAST UPDATE %u-%u-%u\n", header->year, header->month, header->day);
	printf("HEADER SIZE %u\n", header->hsize);
	printf("RECORD SIZE %u\n", header->recsize);
	printf("RECORD NUMBERS %u\n", header->recnum);

	fdesc = header->first;
	while(fdesc != NULL) {
		type = fdesc->type;
		if (type == DTYPE_FLOAT && fdesc->intable == header->recnum) { type = DTYPE_INTEGER; }
		printf("\t'%s' %s(%u)\n", fdesc->name, type_to_str(type), fdesc->length, fdesc->count);
		fdesc = fdesc->next;
	}
}

char * load_header (FILE * fp) {
	char * header = NULL;
	rewind(fp);

	header = calloc(TABLE_HEADER_LENGTH, 1);
	if (!header) { return NULL; }
	if (fread(header, 1, TABLE_HEADER_LENGTH, fp) != TABLE_HEADER_LENGTH) {
		free(header);
		return NULL;
	}

	return header;
}

char * load_fdesc (FILE * fp, uint32_t idx, char * buffer) {
	char * fdesc = NULL;
	char name[12];
	fseek(fp, (long)((idx * FIELD_DESCRIPTOR_LENGTH) + TABLE_HEADER_LENGTH), SEEK_SET);

	if (buffer != NULL) {
		fdesc = buffer;
	} else {
		fdesc = calloc(FIELD_DESCRIPTOR_LENGTH, 1);
	}
	if (!fdesc) { return NULL; }
	if (fread(fdesc, 1, FIELD_DESCRIPTOR_LENGTH, fp) != FIELD_DESCRIPTOR_LENGTH) {
		free(fdesc);
		return NULL;
	}

	strncpy(name, fdesc, 11);
	name[11] = '\0';

	if (fdesc[0] == END_OF_FIELD_DESCRIPTOR) { free(fdesc); return NULL; }
	return fdesc;
}

char * load_record (FILE * fp, uint32_t idx, dtable_header * header, char * buffer) {
	char * data = NULL;
	int allocated = 0;
	fseek(fp, (long)(header->hsize + (idx * header->recsize)), SEEK_SET);
	if (buffer != NULL) {
		data = buffer;
	} else {
		data = calloc(header->recsize, sizeof(*data));
		allocated = 1;
	}
	if (!data) { return NULL; }
	if(fread(data, sizeof(*data), header->recsize, fp) != header->recsize) {
		if (allocated) { free(data); }
		return NULL;
	}

	return data;
}


char * to_utf8 (char * data, uint32_t len, iconv_t idesc) {
	char * outbuff = NULL;
	char * out = NULL;
	char * inbuff = data;
	size_t inlen = len;
	size_t outlen = len * 4;

	outbuff = calloc(outlen, sizeof(*outbuff));
	out = outbuff;
	iconv(idesc, &inbuff, &inlen, &outbuff, &outlen);

	outbuff = out + strlen(out) - 1;
	while (outbuff > out && is_space((unsigned char)*outbuff)) { outbuff--; }
	if (is_space(*outbuff)) { *outbuff = 0x00; }
	outbuff[1] = 0x00;
	outbuff = out + strlen(out) - 1;

	return out;
}

char * _get_text_field (char * data, uint32_t len, iconv_t idesc) {
	return to_utf8(data, len, idesc);
}

double _get_float_field (char * data, uint32_t len, int * nodiv) {
	int is_dec = 0;
	int is_neg = 0;
	int num_started = 0;
	uint32_t i = 0;
	double value = 0;
	float sub = 0;
	unsigned int divider = 1;
	
	*nodiv = 0;
	for (i = 0; i < len; i++) {
		if (!is_num(*(data + i))) { continue; }
		if (*(data + i) == 0x2d && !num_started) { is_neg = 1; continue; }
		num_started = 1;
		if (*(data + i) == 0x2e) { is_dec = 1; continue; }
		if (!is_dec) {
			value *= 10;
			value += (double)((*(data + i) - 0x30) & 0xFF);
			continue;
		}
		divider *= 10;
		sub *= 10;
		sub += (float)((*(data + i) - 0x30) & 0xFF);
	}
	if (sub == 0.0) {
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

	if (is_space(*data)) { 
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
	block  = (uint32_t)((*data) & 0xFF);
	block |= (uint32_t)((*(data + 1)) & 0xFF) << 8;
	block |= (uint32_t)((*(data + 2)) & 0xFF) << 16;
	block |= (uint32_t)((*(data + 3)) & 0xFF) << 24;
	return block;
}

struct tm _get_datetime_field (char * data, uint32_t len) {
	struct tm datetime = {0};
	/* todo */
	return datetime;
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
		pos += hcurrent->length;
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
	if (!record) { return NULL; }

	record->deleted = *data == RECORD_DELETED ? 1 : 0;

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
				if(!fcurrent->not_init && header->memo) {
					fcurrent->bmemo = memo_get_block(header->memo, fcurrent->memo, &(header->mheader));
					fcurrent->text = to_utf8(fcurrent->bmemo->data, fcurrent->bmemo->length, header->idesc);
				}

				break;
			case DTYPE_DATE:
				fcurrent->date = _get_date_field(data + pos, hcurrent->length, &(fcurrent->not_init));
				break;
			case DTYPE_INTEGER:
				fcurrent->integer = _get_int_field(data + pos, hcurrent->length);
				fcurrent->number = (double)fcurrent->integer;
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
				fcurrent->text = _get_text_field(data + pos, hcurrent->length, header->idesc);
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
		if (c->bmemo) { memo_free_block(c->bmemo); }
		free(c);
		c = tmp;
	}
	free(record);
}

dtable_record * get_record(dtable * table, uint32_t idx) {
	dtable_record * record = NULL;

	if (table == NULL) { return NULL; }
	if (idx >= table->header->recnum) { return NULL; }
	table->buffer = load_record(table->fp, idx, table->header, table->buffer);
	if (!table->buffer) { return NULL; }
	record = parse_record(table->buffer, table->header);
	if (record) {
		table->current_record = idx;
		record->index = idx;
	}
	return record;
}

dtable_field * get_field(dtable_record * record, char * name) {
	dtable_field * field = NULL;
	if (!record || !name) { return NULL; }
	field = record->first;

	while(field) {
		if (strcmp(field->descriptor->name, name) == 0) { return field; }
		field = field->next;
	}

	return NULL;
}

dtable_record * get_first_record(dtable * table) {
	return get_record(table, 0);
}

dtable_record * get_last_record(dtable * table) {
	if (!table) { return NULL; }
	return get_record(table, table->header->recnum - 1);
}

dtable_record * get_next_record(dtable * table) {
	if (!table) { return NULL; }
	return get_record(table, ++(table->current_record));
}

dtable_record * get_previous_record(dtable * table) {
	if (!table) { return NULL; }
	return get_record(table, --(table->current_record));
}

dtable * open_dtable(char * dbf, char * dbt) {
	dtable * table = NULL;
	dtable_fdesc * fdesc = NULL;
	FILE * fp = NULL;
	uint32_t i = 0;

	table = calloc(1, sizeof(*table));
	if (!table) { return NULL; }

	fp = fopen(dbf, "r");
	if (!fp) { free(table); return NULL; }

	table->buffer = load_header(fp);
	if (!table->buffer) { free(table); fclose(fp); return NULL; }
	table->current_record = -1;
	table->header = parse_header(table->buffer);
	table->fp = fp;
	free(table->buffer);
	table->buffer = NULL;

	if (!table->header) { close_dtable(table); return NULL; }
	
	table->header->idesc = iconv_open("UTF-8", "ISO-8859-1");
	if (table->header->idesc == (iconv_t)-1) {
		close_dtable(table);
		return NULL;		
	}
	/* if fail open dbt, we dont close db, just work without it */
	if (table->header->memo_file && dbt) {
		fp = fopen(dbt, "r");
		if (fp) {
			table->header->memo = fp;
			memo_get_header(table->header->memo, &(table->header->mheader));
		}
	}

	/* load fdesc */
	i = 0;
	while((table->buffer = load_fdesc(table->fp, i++, table->buffer))) {
		fdesc = parse_fdesc(table->buffer, fdesc, table->header->idesc);
		if (!fdesc) { close_dtable(table); return NULL; }
		if (!table->header->first) { table->header->first = fdesc; }
	}
	free(table->buffer);
	table->buffer = NULL;

	/* preflight dtable */
	for(i = 0; i < table->header->recnum; i++) {
		table->buffer = load_record(table->fp, i, table->header, table->buffer);
		if (!table->buffer) { break; }
		preflight_record(table->buffer, table->header);
	}

	return table;
}

void close_dtable(dtable * table) {
	if (table->fp) { fclose(table->fp); }
	if (table->buffer) { free(table->buffer); }
	free_header(table->header);
	free(table);
}
