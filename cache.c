#include "dbase.h"
#include "cache.h"

dbase_cache * cache_init (size_t cache_size) {
	dbase_cache * cache = NULL;

	cache = calloc(1, sizeof(*cache));
	if (cache == NULL) { return NULL; }

	cache->records = calloc(cache_size, sizeof(*(cache->records)));
	if (cache->records == NULL) { free(cache); return NULL; }
	
	cache->size = cache_size;
	cache->used = 0;

	return cache;	
}

void cache_destroy (dbase_cache * cache) {
	size_t i = 0;
	
	if (!cache) { return; }
	
	for (i = 0; i < cache->size; i++) {
		if ((cache->records + i)->record != NULL) {
			free_record((dtable_record *)(cache->records + i)->record);
		}
	}
	free(cache->records);
	free(cache);
}

void cache_get_space (dbase_cache * cache) {
	size_t i = 0;
	size_t selected = 0;
	unsigned int most = 0;
	dtable_record * record;
	
	if(!cache) { return; }

	if (cache->used < cache->size) { return ; }

	for (i = 0; i < cache->size; i++) {
		if ((cache->records + i)->record != NULL) {
			if (most < (cache->records + i)->access_count) {
				selected = i;
				most = (cache->records + i)->access_count;
			}	
		}
	}

	if ((cache->records + selected)->record == NULL) { return; }

	record = (dtable_record *)(cache->records + selected)->record;

	record->cached = 0;
	if(!record->freed) {
		free_record(record);
	}
	(cache->records + selected)->record = NULL;
	(cache->records + selected)->idx = 0;
	(cache->records + selected)->access_count = 0;

	cache->used--;
}

void cache_add_record (dbase_cache * cache, uint32_t idx, dtable_record * record) {
	size_t i = 0;
	int added = 0;

	if (!cache) { return; }

	cache_get_space(cache);
	for (i = 0; i < cache->size; i++) {
		if ((cache->records + i)->record == NULL) {
			(cache->records + i)->record = record;
			(cache->records + i)->access_count = 1;
			(cache->records + i)->idx = idx;
			added = 1;
			break;
		}
	}

	if (!added) {
		record->cached = 0;
		return;
	}
	record->cached = 1;
	cache->used++;
}

dtable_record * cache_get_record(dbase_cache * cache, uint32_t idx) {
	size_t i = 0;
	dtable_record * record = NULL;
	
	if (!cache) { return NULL; }
	
	for (i = 0; i < cache->size; i++) {
		if ((cache->records + i)->record == NULL) { continue; }
		if ((cache->records + i)->idx == idx) {
			record = (cache->records + i)->record;
			(cache->records + i)->access_count++;
			break;
		}
	}

	return record;
}











