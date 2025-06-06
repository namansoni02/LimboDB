#pragma once
#include <iostream>

struct RecordID {
public:
    int page_id;  // Page ID where the record is stored
    int slot_id;  // Slot ID within the page

    RecordID(int p_id = -1, int s_id = -1) : page_id(p_id), slot_id(s_id) {}

    bool operator==(const RecordID& other) const {
        return page_id == other.page_id && slot_id == other.slot_id;
    }

    bool operator!=(const RecordID& other) const {
        return !(*this == other);
    }

    bool is_valid() const {
        return page_id >= 0 && slot_id >= 0;
    }

    int encode() const {
        return (page_id << 16) | slot_id;
    }

    static RecordID decode(int record_id) {
        int page_id = record_id >> 16;
        int slot_id = record_id & 0xFFFF;
        return RecordID(page_id, slot_id);
    }

    friend std::ostream& operator<<(std::ostream& os, const RecordID& rid) {
        os << "[Page: " << rid.page_id << ", Slot: " << rid.slot_id << "]";
        return os;
    }
};
