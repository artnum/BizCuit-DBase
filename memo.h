#ifndef _MEMO_H__
#define _MEMO_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct s_memo_header memo_header;
typedef struct s_memo_block memo_block;

struct s_memo_header {
	uint32_t next;
	uint32_t bs;
};

struct s_memo_block {
	uint32_t type;
	uint32_t length;

	char * data;
};

void memo_get_header (FILE * fp, memo_header * header);
memo_block * memo_get_block (FILE * fp, uint32_t index, memo_header * header);
void memo_free_block(memo_block * block);

#endif
