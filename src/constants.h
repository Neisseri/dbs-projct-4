#pragma once

#include <tuple>
#include <cstring>
#include <vector>

// foreign_key_name, reference_table_name, field_names, reference_field_names
typedef std::tuple<std::string, std::string, std::vector<std::string>, std::vector<std::string>> ForeignKey;

#define UPDATE1 "UPDATE T2 SET ID2 = 12 WHERE T2.ID1 = 10 AND T2.ID2 = 10;"