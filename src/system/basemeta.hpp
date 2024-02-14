#pragma once

#include <cstring>
#include <vector>

#include "constants.h"

class BaseMeta { // meta data for all data tables of this database
public:
    BaseID base_id;
    std::string name;
    std::vector<std::pair<std::string, TableID>> tables; // table name, table id

    explicit BaseMeta() = default;
    explicit BaseMeta(BaseID base_id, std::string name) : 
        base_id(base_id), name(std::move(name)) {}
};