#ifndef _CACHE_H__
#define _CACHE_H__

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

typedef struct s_dbase_cache dbase_cache;
typedef struct s_dbase_cache_record dbase_cache_record;

struct s_dbase_cache_record {
	uint32_t idx;
	void * record;
	unsigned int access_count;
};


struct s_dbase_cache {
	size_t size;
	size_t used;
	dbase_cache_record * records;
};

dtable_record * cache_get_record(dbase_cache * cache, uint32_t idx); 
void cache_add_record (dbase_cache * cache, uint32_t idx, dtable_record * record); 
void cache_get_space (dbase_cache * cache);
void cache_destroy (dbase_cache * cache);
dbase_cache * cache_init (size_t cache_size);

#endif
