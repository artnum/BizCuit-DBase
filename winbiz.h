#ifndef _WINBIZ_H_
#define _WINBIZ_H_

#include "dbase.h"
#include "account.h"

typedef struct s_winbiz winbiz;

struct s_winbiz {
	dtable * ecriture;
	dtable * articles;
};

#endif
