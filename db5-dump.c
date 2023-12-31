#include <stdio.h>
#include <stdlib.h>
#include "dbase.h"

int main (int argc, char ** argv) {
	dtable * table = NULL;
	dtable_record * record = NULL;
	dtable_field * field = NULL;
	char datebuff[11];

	if (argc < 2) { return 0; }
	if (argc == 2) {
		table =	open_dtable(argv[1], NULL, DTABLE_OPT_NO_CHAR_TO_INT);
	} else {
		table = open_dtable(argv[1], argv[2], DTABLE_OPT_NO_CHAR_TO_INT);
	}
	if (!table) { return 0; }
	dump_header(table->header);
	
	while((record = get_next_record(table)) != NULL) {	
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
					printf(" %ld\n", field->integer);
					break;
				case DTYPE_FLOAT:
					printf(" %.4f\n", field->number);
					break;
				case DTYPE_MEMO:
					if (field->not_init) {
						printf(" unset\n");
						break;
					}
					printf(" @block %d [%s]\n", field->memo, field->text);
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
	close_dtable(table);
	return 0;
}
