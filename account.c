#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "account.h"
#include "dbase.h"

accounting * init_accounting() {
	return calloc(1, sizeof(accounting));
}

void free_accounting(accounting * ctx) {
	account * tmp = NULL;
	account * next = NULL;
	if (!ctx) { return; }
	tmp = ctx->first;
	while (tmp) {
		next =  tmp->next;
		free(tmp);
		tmp = next;
	}
	free(ctx);
}

void dump_accounting(accounting * ctx) {
	account * tmp = NULL;
	account_op * op = NULL;
	int ops = 0;
	if (!ctx) { return; }

	tmp = ctx->first;
	while(tmp) {
		ops = 0;
		op = tmp->first;
		while(op) {
			ops++;
			op = op->next;
		}
		printf("%s\t%18.2f\t%8d\n", tmp->name, tmp->amount, ops);
		tmp = tmp->next;
	}
}

char * _strcpy (char * src) {
	char * dest = NULL;
	size_t len = 0;

	len = strlen(src);
	if (len == 0) { return NULL; }
	dest = calloc(len + 1, sizeof(*dest));
	if (!dest) { return NULL; }
	strncpy(dest, src, len);
	return dest;
}

account_op * dbase_to_op (dtable_record * record) {
	account_op * op = NULL;
	dtable_field * field = NULL;

	op = calloc(1, sizeof(*op));
	if (!op) { return NULL; }
	field = get_field(record, "MULTIPLE");
	if (field->boolean) {
		field = get_field(record, "ECR_NOLINE");
		if (field->number == 1) { free(op); return NULL; }
	}

	field = get_field(record, "NUMÉRO");
	op->number = field->number;
	field = get_field(record, "MONTANT");
	op->amount = field->number;
	field = get_field(record, "LIBELLÉ");
	op->wording = _strcpy(field->text);
	field = get_field(record, "DATE");
	memcpy(&(op->date), &(field->date), sizeof(op->date));
	field = get_field(record, "CPT_DÉBIT");
	op->debit = _strcpy(field->text);
	field = get_field(record, "CPT_CRÉDIT");
	op->credit = _strcpy(field->text);
	field = get_field(record, "ECR_TVATX");
	op->tax_value = field->number;
	field = get_field(record, "ECR_TVAMNT");
	op->tax_amount = field->number;

	op->group = NULL;
	op->next = NULL;

	return op;
}

void add_op_to_list(account_op_list * list, account_op *op) {
	account_op * tmp;

	if (!list || !op) { return; }
	if (!list->first) { list->first = op; return; }

	tmp = list->first;
	while(tmp) {
		if (tmp->number == op->number) {
			if (!tmp->group) {
				tmp->group = op;
				return;
			}
			tmp = tmp->group;
		}
		if (tmp->next == NULL) {
			list->count++;
			tmp->next = op;
			return;
		}
		tmp = tmp->next;
	}
}

account_op_list * create_op_list () {
	return calloc(1, sizeof(account_op_list));
}

void dump_op (account_op * op) {
	account_op * tmp;
	char date[11] = { 0x00 };
	strftime(date, 11, "%d.%m.%Y", &(op->date));
	printf("# %08d %s %s\n", op->number, date, op->wording);
	printf("\t%s\t%s\t%12.2f\tTVA %3.1f\t%12.2f\n", 
			op->debit, op->credit, op->amount, op->tax_value, op->tax_amount);
	if (op->group) {
		tmp = op->group;
		while (tmp) {
			printf("\t%s\t%s\t%12.2f\tTVA %3.1f\t%12.2f\n", 
					tmp->debit, tmp->credit, tmp->amount,
					tmp->tax_value, tmp->tax_amount);
			tmp = tmp->next;
		}
	}
	printf("\n");	
}

void dump_op_list (account_op_list * list) {
	account_op * op;

	op = list->first;

	while (op) {
		dump_op(op);
		op = op->next;
	}
}

void free_op(account_op * op) {
	account_op * tmp = NULL;
	account_op * tmp2 = NULL;

	if (op->wording) { free(op->wording); }
	if (op->debit) { free(op->debit); }
	if (op->credit) { free(op->credit); }
	
	tmp = op->group;
	while(tmp) {
		tmp2 = tmp->next;
		free_op(tmp);
		tmp = tmp2;
	}
	free(op);
}

void free_op_list (account_op_list * list) {
	account_op * tmp = NULL;
	account_op * tmp2 = NULL;

	tmp = list->first;
	while(tmp) {
		tmp2 = tmp->next;
		free_op(tmp);
		tmp = tmp2;
	}
	free(list);
}

account * add_account (accounting * ctx, char * name) {
	account * a = NULL;
	account * tmp = NULL;
	size_t name_len = ACCOUNT_MAX_NAME_LENGTH;

	a = calloc(1, sizeof(*a));
	if (!a) { return NULL; }

	name_len = strlen(name);
	if (name_len > ACCOUNT_MAX_NAME_LENGTH) { name_len = ACCOUNT_MAX_NAME_LENGTH; }

	strncpy(a->name, name, name_len);
	a->amount = 0;
	a->next = NULL;

	if (!ctx->first) { ctx->first = a; }
	else {
		tmp = ctx->first;
		while(tmp->next) {
			tmp = tmp->next;
		}
		tmp->next = a;
	}

	return a;
}

account * find_account (accounting * a, char * name) {
	char n[ACCOUNT_MAX_NAME_LENGTH];
	size_t name_len = ACCOUNT_MAX_NAME_LENGTH;
	account * ac = NULL;

	name_len = strlen(name);
	if (name_len > ACCOUNT_MAX_NAME_LENGTH) { name_len = ACCOUNT_MAX_NAME_LENGTH; }

	strncpy(n, name, ACCOUNT_MAX_NAME_LENGTH);

	ac = a->first;
	while (ac) {
		if (strncmp(ac->name, n, name_len) == 0) { return ac; }
		ac = ac->next;
	}
	return ac;
}

void add_op_to_account (account * a, void * operation) {
	/*account_op * op = NULL;
	account_op * tmp = NULL;

	op = calloc(1, sizeof(*op));
	op->op = operation;
	op->next = NULL;

	if (a->first == NULL) { a->first = op; return; }

	tmp = a->first;
	while (tmp->next != NULL) { tmp = tmp->next; }
	tmp->next = op;
*/
}

void add_to_account(accounting * a, char * name, double value, void * op) {
	account * ac = NULL;

	ac = find_account(a, name);
	if (!ac) { ac = add_account(a, name); }
	if (ac) {
		ac->amount += value;
	}

	add_op_to_account(ac, op);
}


void sub_to_account(accounting * a, char * name, double value, void * op) {
	account * ac = NULL;

	ac = find_account(a, name);
	if (!ac) { ac = add_account(a, name); }
	if (ac) {
		ac->amount -= value;
	}

	add_op_to_account(ac, op);
}

void account_double_write(accounting * a, char * left, char * right, double amount, void * op) {
	add_to_account(a, left, amount, op);
	sub_to_account(a, right, amount, op);
}
