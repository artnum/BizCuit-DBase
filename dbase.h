#ifndef _DBASE_H_
#define _DBASE_H_

#include "memo.h"

#define TABLE_HEADER_LENGTH	32
#define FIELD_DESCRIPTOR_LENGTH	32

#define END_OF_FIELD_DESCRIPTOR	0x0D

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
	memo_block * bmemo;

	uint8_t not_init;

	dtable_field * next;
};

#endif
