
/*
 * Performance event collection helper functions
 */

/* 
 * Author:  B.A.Belson
 * Version: 1.0
 * Date:    8 July 2021
 */

#include "perf/pe_assist_cpp.h"

void pe_report_summaries(std::ostream& os, const pe_event_collection_sum_set& ss, 
  size_t summary_count, const char* summary_names[], const std::vector<std::string>& field_list) {
	std::string sep = ",";
	if (field_list.size() > 0) {
		os << "test";
		for (size_t i = 0; i < field_list.size(); i++) {
			os << sep << field_list[i];
		}
		os << std::endl;
		for (size_t s = 0; s < summary_count; s++) {
			os << summary_names[s];
			for (size_t i = 0; i < field_list.size(); i++) {
        os << sep;
				auto e = pe_extract_summary_value(&ss.items[s].data, field_list[i].c_str(), ss.items[s].count);
				switch (e.which) {
					case 0: 
						os << e.v;
						break;
					case 1:
						os << e.r;
						break;
				}
			}
			os << std::endl;
		}
	}
}

