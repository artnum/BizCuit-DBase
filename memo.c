#include "memo.h"

void memo_get_header (FILE * fp, memo_header * header) {
	char data[8] = { 0x00 };

	header->next = 0;
	header->bs = 0;

	fread(data, sizeof(data[0]), 8, fp);

	header->next  = (uint32_t)(data[0] & 0xFF) << 24;
	header->next |= (uint32_t)(data[1] & 0xFF) << 16;
	header->next |= (uint32_t)(data[2] & 0xFF) << 8;
	header->next |= (uint32_t)(data[3] & 0xFF);

	header->bs  = (uint32_t)(data[4] & 0xFF) << 24;
	header->bs |= (uint32_t)(data[5] & 0xFF) << 16;
	header->bs |= (uint32_t)(data[6] & 0xFF) << 8;
	header->bs |= (uint32_t)(data[7] & 0xFF);
}

memo_block * memo_get_block (FILE * fp, uint32_t index, memo_header * header) {
	char data[8] = { 0x00 };
	memo_block * block = NULL;
	
	block = calloc(1, sizeof(*block));

	fseek(fp, header->bs * index, SEEK_SET);
	fread(data, sizeof(data[0]), 8, fp);


	block->type = 0;
	block->length = 0;

	block->type  = (uint32_t)(data[0] & 0xFF) << 24;
	block->type |= (uint32_t)(data[1] & 0xFF) << 16;
	block->type |= (uint32_t)(data[2] & 0xFF) << 8;
	block->type |= (uint32_t)(data[3] & 0xFF);

	block->length  = (uint32_t)(data[4] & 0xFF) << 24;
	block->length |= (uint32_t)(data[5] & 0xFF) << 16;
	block->length |= (uint32_t)(data[6] & 0xFF) << 8;
	block->length |= (uint32_t)(data[7] & 0xFF);

	block->data = calloc(block->length, sizeof(*(block->data)));
	fread(block->data, sizeof(*(block->data)), block->length, fp);

	return block;
}

void memo_free_block(memo_block * block) {
	if (block->data) { free(block->data); }
	free(block);
}
