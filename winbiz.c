#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "winbiz.h"
#include "account.h"

void close_winbiz(winbiz * ctx) {
	if (!ctx) { return; }
	if (ctx->ecriture) { close_dtable(ctx->ecriture); }
	if (ctx->articles) { close_dtable(ctx->articles); }
	free(ctx);
}

winbiz * init_winbiz(char * root) {
	winbiz * ctx = NULL;
	char * path = NULL;
	char * path2 = NULL;
	size_t len = 0;

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) { return NULL; }

	len = strlen(root);
	path = calloc(len + 30, sizeof(*path));
	if (!path) { free(ctx); return NULL; }
	path2 = calloc(len + 30, sizeof(*path2));
	if (!path2) { free(path); free(ctx); return NULL; }

	sprintf(path, "%s/ecriture.dbf", root);
	ctx->ecriture = open_dtable(path, NULL);

	sprintf(path, "%s/articles.dbf", root);
	sprintf(path2, "%s/articles.FPT", root);
	ctx->articles = open_dtable(path, path2);

	free(path);
	free(path2);

	return  ctx;
}

int main (int argc, char ** argv) {
	void * dlh = NULL;
	winbiz * ctx = NULL;
	dtable_record * record = NULL;
	account_op_list * operations;
	account_op * op;
	char * left = NULL;
	char * right = NULL;
	double tva = 0;
	double amount = 0;
	int (*process)(account_op_list *);

	if (argc < 2) { return 0; }
	ctx = init_winbiz(argv[1]);
	if (!ctx) { return 0; }

	operations = create_op_list();

	while ((record = get_next_record(ctx->ecriture)) != NULL) {
		op = dbase_to_op(record);
		if (op) {
			add_op_to_list(operations, op);
		}
		free_record(record);
	}

	dlh = dlopen(argv[2], RTLD_LAZY);
	if (!dlh) {
		printf("dl not av");
	} else {
		*(void **) (&process) = dlsym(dlh, "process_account_list");
		(*process)(operations);
		dlclose(dlh);
	}
	free_op_list(operations);

	close_winbiz(ctx);
}
