#ifndef _DBASE_H_
#define _DBASE_H_

#include <stdint.h>
#include <time.h>
#include <iconv.h>
#include "memo.h"

#define TABLE_CACHE_SIZE	100

#define TABLE_HEADER_LENGTH	32
#define FIELD_DESCRIPTOR_LENGTH	32

#define END_OF_FIELD_DESCRIPTOR	0x0D
#define RECORD_DELETED	'*'

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

#define is_int(c) (((c) >= 0x30 && (c) <= 0x39)) 
#define is_num(c) (((c) >= 0x30 && (c) <= 0x39) || (c) == 0x2e || (c) == 0x2d) 
#define is_space(x) ((x) == 0x20 || (x) == 0x09 || (x) == 0x0A || (x) == 0x0D)

typedef struct s_dtable dtable;
typedef struct s_dtable_header dtable_header;
typedef struct s_dfdesc dtable_fdesc;
typedef struct s_dfield dtable_field;
typedef struct s_dreco dtable_record;

#include "cache.h"

struct s_dtable {
	FILE * fp;
	char * buffer;
	dtable_header * header;
	long int current_record;
	dbase_cache * cache;
};

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

	FILE * memo;
	memo_header mheader;

	iconv_t idesc;

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
	uint8_t cached;
	uint8_t freed;
	dtable_field * first;
};

struct s_dfield {
	dtable_fdesc * descriptor;
	
	char * text;
	long int integer;
	double number;
	uint8_t boolean;
	struct tm date;
	uint32_t memo;
	memo_block * bmemo;

	uint8_t not_init;

	dtable_field * next;
};

dtable_fdesc * parse_fdesc (char * data, dtable_fdesc * previous, iconv_t idesc);
dtable_header * parse_header (char * data); 
void free_header (dtable_header * header); 
void dump_header (dtable_header * header); 
char * load_header (FILE * fp); 
char * load_fdesc (FILE * fp, uint32_t idx, char * buffer); 
char * load_record (FILE * fp, uint32_t idx, dtable_header * header, char * buffer); 
void preflight_record (char * data, dtable_header * header); 
dtable_record * parse_record (char * data, dtable_header * header); 
void free_record (dtable_record * record);

dtable * open_dtable(const char * dbf, const char * dbt); 
dtable_record * get_record(dtable * table, uint32_t idx);
dtable_record * get_first_record(dtable * table); 
dtable_record * get_last_record(dtable * table);
dtable_record * get_next_record(dtable * table);
dtable_record * get_previous_record(dtable * table); 
dtable_field * get_field(dtable_record * record, const char * name); 
long int get_int_field(dtable_record * record, const char * name);
long int coerce_int(dtable_field * field);
char * get_str_field(dtable_record * record, const char * name);
char * coerce_str(dtable_field * field);
void close_dtable(dtable * table); 
#endif
