// CSV parser for task definitions

#ifndef CSV_PARSER_H_
#define CSV_PARSER_H_

#include "models.h"
#include <vector>
#include <string>

namespace parsers {

// Parse tasks from a CSV file
std::vector<models::Task> parse_tasks_csv(const std::string& csv_path);

// Validate that all task dependencies exist and there are no circular dependencies
void validate_task_dependencies(const std::vector<models::Task>& tasks);

} // namespace parsers

#endif // CSV_PARSER_H_
