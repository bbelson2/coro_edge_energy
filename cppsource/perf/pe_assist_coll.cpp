
/*
 * Performance event collection helper functions
 */

/* 
 * Author:  B.A.Belson
 * Version: 1.0
 * Date:    8 July 2021
 */

#include <perf/pe_assist.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

void pe_event_collection_clear(pe_event_collection* c) {
  memset(c, 0, sizeof(pe_event_collection));
  for (int i = 0; i < PE_MAX_EVENT_VALUES; i++) {
    c->values[i].value = 0; 
  }
  for (int i = 0; i < PE_MAX_EVENT_RATIOS; i++) {
    c->ratios[i].value = 0; 
  }
}

void pe_event_collection_populate(pe_event_collection* c,
  size_t values_count, const char* value_names[], const char* value_titles[], 
  size_t ratios_count, const char* ratio_names[], const char* ratio_titles[])
{
  assert(values_count > 0 && values_count <= (size_t)PE_MAX_EVENT_VALUES);  
  assert(ratios_count > 0 && ratios_count <= (size_t)PE_MAX_EVENT_RATIOS);  
  c->values_populated = values_count;
  for (size_t i = 0; i < values_count; i++) {
    c->values[i].name = value_names[i];
    c->values[i].title = value_titles[i];
  }
  c->ratios_populated = ratios_count;
  for (size_t i = 0; i < ratios_count; i++) {
    c->ratios[i].name = ratio_names[i];
    c->ratios[i].title = ratio_titles[i];
  }
}

pe_extracted_value pe_extract_summary_value(const pe_event_collection* ec, 
	const char* field_name, size_t count)
{
  if (count) {
    for (int i = 0; i < ec->values_populated; i++) {
      if (!strcmp(field_name, ec->values[i].name)) {
        pe_extracted_value ev = { ec->values[i].value / (long long int)count, (double)0.0, 0 };
        return ev;
      }
    }
    for (int i = 0; i < ec->ratios_populated; i++) {
      if (!strcmp(field_name, ec->ratios[i].name)) {
        pe_extracted_value ev = {(long long int)0, ec->ratios[i].value / (double)count, 1};
        return ev;
      }
    }
  }
  pe_extracted_value ev0 = {(long long int)0, (double)0.0, -1};
	return ev0;
}

void pe_event_collection_sum_set_init(pe_event_collection_sum_set* ss,
  size_t count, const char* sum_names[]) {
  ss->count = count;
  ss->items = (pe_event_collection_sum*)malloc(sizeof(pe_event_collection_sum) * count);
  memset(ss->items, 0, sizeof(pe_event_collection_sum) * count);
  for (size_t i = 0; i < count; i++) {
    ss->items[i].name = sum_names[i];
  }
}

void pe_event_collection_sum_set_term(pe_event_collection_sum_set* ss) {
  free(ss->items);
  memset(ss, 0, sizeof(pe_event_collection_sum_set));
}

int pe_event_collection_sum_set_find_index(pe_event_collection_sum_set* ss, 
  const char* sum_name) {
  for (size_t i = 0; i < ss->count; i++) {
    if (!strcmp(ss->items[i].name, sum_name)) {
      return (int)i;
    }
  }
  return -1;
}

void pe_event_collection_sum_set_append(pe_event_collection_sum_set* ss,  
  const char* sum_name, const pe_event_collection* ec) {

	int s = pe_event_collection_sum_set_find_index(ss, sum_name);
  if (s < 0) {
    return;
  }
  pe_event_collection_sum* d = ss->items + s;
	d->count++;
	for (int i = 0; i < ec->values_populated; i++) { 
		d->data.values[i].name = ec->values[i].name;
		d->data.values[i].title = ec->values[i].title;
		d->data.values[i].value += ec->values[i].value;
	}
	d->data.values_populated = ec->values_populated;
	for (int i = 0; i < ec->ratios_populated; i++) {
		d->data.ratios[i].name = ec->ratios[i].name;
		d->data.ratios[i].title = ec->ratios[i].title;
		d->data.ratios[i].value += ec->ratios[i].value;
	}
	d->data.ratios_populated = ec->ratios_populated;
}

