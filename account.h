#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

#include <time.h>
#include "dbase.h"

#define ACCOUNT_MAX_NAME_LENGTH	10

typedef struct s_account account;
typedef struct s_accounting accounting;
typedef struct s_account_op account_op;
typedef struct s_account_op_list account_op_list;

struct s_account_op_list {
	unsigned int count;
	account_op * first;
};

struct s_account_op {
	unsigned int number;
	char * wording;
	char * debit;
	char * credit;
	double amount;
	struct tm date;
	double tax_amount;
	double tax_value;
	
	account_op * group;	
	account_op * next;
};

struct s_accounting {
	account * first;
};
struct s_account {
	char name[ACCOUNT_MAX_NAME_LENGTH];
	double amount;

	account * next;
	account_op * first;
};

accounting * init_accounting();
account * add_account (accounting * ctx, char * name);
account * find_account (accounting * ctx, char * name); 
void add_to_account(accounting * ctx, char * name, double value, void * op);
void sub_to_account(accounting * ctx, char * name, double value, void * op); 
void account_double_write(accounting * ctx, char * left, char * right, double amount, void * op); 
void dump_accounting(accounting * ctx); 
void free_accounting(accounting * ctx); 


account_op * dbase_to_op (dtable_record * record); 
void add_op_to_list(account_op_list * list, account_op *op); 
account_op_list * create_op_list ();
void dump_op_list (account_op_list * list);
void free_op_list (account_op_list * list); 
void free_op(account_op * op); 

#endif
