#pragma once

#include <vector>
#include <memory>

#include "field.hpp"
#include "constants.h"

#include "../filesystem/bufmanager/BufPageManager.h"
#include "../filesystem/fileio/FileManager.h"
#include "../filesystem/utils/pagedef.h"

class Record {
public:
    std::vector<std::shared_ptr<Field>> fields;

    explicit Record() = default;
    explicit Record(std::vector<std::shared_ptr<Field>> fields) : fields(std::move(fields)) {}

    ~Record() = default;

    void store(int fileID, BufPageManager* bpm, int pageID, int start_pos) {
        int index;
        BufType b;
        if (start_pos == 0) { // alloc a new page
            b = bpm->allocPage(fileID, pageID, index, true);
        } else { // get an existing page
            b = bpm->getPage(fileID, pageID, index);
        }
        int offset = start_pos;
        for (const auto &field : fields) {
            field->store(b, offset);
        }
        bpm->markDirty(index);
        bpm->writeBack(index);
    }

    void load(int fileID, BufPageManager* bpm, int pageID, int start_pos) {
        int index;
        BufType b = bpm->getPage(fileID, pageID, index);
        int offset = start_pos;
        for (auto &field : fields) {
            field->load(b, offset);
        }
        bpm->access(index);
        bpm->release(index);
    }

    std::string toString() const {
        std::string str;
        for (const auto &field : fields) {
            str += field->toString();
            if (field != fields.back()) {
                str += ",";
            }
        }
        str += '\n';
        return str;
    }
};