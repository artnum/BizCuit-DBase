#include "account.h"

#include <string.h>
#include <jansson.h>
#include <curl/curl.h>

int process_account_list (account_op_list * list) 
{
	CURL *curl;
	char ref[80];
	char * body = NULL;
	account_op * op = NULL;
	account_op * group = NULL;
	json_t * root = NULL;
	json_t * value = NULL;
	json_t * array = NULL;
	json_t * entry = NULL;
	struct curl_slist * headers = NULL;

	if (!list) { return 0; }
	curl = curl_easy_init();
	if (!curl) { return 0; }
	headers = curl_slist_append(headers, "Authorization: Bearer eyJraWQiOiI2ZGM2YmJlOC1iMjZjLTExZTgtOGUwZC0w");
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_URL, "https://zenix/blank.php");
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(curl, CURLOPT_POST, 1);

	root = json_object();
	op = list->first;
	while(op) {
		json_object_set(root, "entries", value);

		memset(ref, 0x00, 80);
		snprintf(ref, 80, "wbz-%08d-2000", op->number);
		value = json_string(ref);
		if (!value) {
			json_object_clear(root);
			json_decref(root);
			op = op->next;
			continue;
		}
		json_object_set_new(root, "reference_nr", value);

		value = NULL;
		if (op->group) {
			value = json_string("manual_compound_entry");
		} else {
			value = json_string("manual_single_entry");
		}
		if (!value) {
			json_object_clear(root);
			json_decref(root);
			op = op->next;
			continue;
		}
		json_object_set_new(root, "type", value);

		value = NULL;

		strftime(ref, 80, "%Y-%m-%d", &(op->date));
		value = json_string(ref);
		if (!value) {
			json_object_clear(root);
			json_decref(root);
			op = op->next;
			continue;
		}
		json_object_set_new(root, "date", value);

		array = json_array();

		entry = json_object();
		value = json_string(op->debit);
		json_object_set_new(entry, "debit_account_id", value);
		value = json_string(op->credit);
		json_object_set_new(entry, "credit_account_id", value);
		value = json_string(op->wording);
		json_object_set_new(entry, "description", value);
		value = json_real(op->amount);
		json_object_set_new(entry, "amount", value);
		json_array_append_new(array, entry);

		group = op->group;
		while(group) {
			entry = json_object();
			value = json_string(group->debit);
			json_object_set_new(entry, "debit_account_id", value);
			value = json_string(group->credit);
			json_object_set_new(entry, "credit_account_id", value);
			value = json_string(group->wording);
			json_object_set_new(entry, "description", value);
			value = json_real(group->amount);
			json_object_set_new(entry, "amount", value);
			json_array_append_new(array, entry);

			group = group->next;
		}

		json_object_set_new(root, "entries", array);	

		body = json_dumps(root, JSON_COMPACT);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
		curl_easy_perform(curl);
		
		free(body);
		op = op->next;
		json_object_clear(root);
	}
	json_decref(root);
	curl_easy_cleanup(curl);
	return 0;
}

