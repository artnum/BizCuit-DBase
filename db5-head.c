#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <dirent.h>

#include "dbase.h"

int is_dbf(char * path) {
	return strcasecmp("dbf", path + strlen(path) - 3) == 0;
}

int main (int argc, char ** argv) {
	DIR * dh = NULL;
	struct dirent * f = NULL;
	char path[1024] = { 0x00 };
	dtable * table;
	if (argc < 2) { return 0; }

	dh = opendir(argv[1]);
	if (!dh) { return 0; }

	while ((f = readdir(dh)) != NULL) {
		sprintf(path, "%s/%s", argv[1], f->d_name);
		if (!is_dbf(path)) { continue; }
		table = open_dtable(path, NULL, 0);
		if (!table) { continue; }
		if (table->header->recnum == 0) { close_dtable(table); continue; }
		printf("### %s\n", f->d_name);
		dump_header(table->header);
		close_dtable(table);
	}
	closedir(dh);

	return 0;
}
