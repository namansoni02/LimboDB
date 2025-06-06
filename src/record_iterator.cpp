#include "../include/record_iterator.h"
#include <iostream>

using namespace std;

#define DEBUG_PREFIX "[DEBUG][RECORD_ITERATOR] "

// Remove 'valid' member and all logic related to it

RecordIterator::RecordIterator(DiskManager& disk_manager) 
    : disk(disk_manager), current_page_id(0), current_slot_id(0) {
    try {
        page = disk.read_page(current_page_id);
        cout << DEBUG_PREFIX << "Initialized at page " << current_page_id << ".\n";
        load_next_valid_record();
    } catch (...) {
        cout << DEBUG_PREFIX << "No pages available at initialization.\n";
        current_page_id = -1; // No valid page => end iterator
    }
}

void RecordIterator::load_next_valid_record() {
    while (current_page_id >= 0) {
        uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
        uint16_t slot_count = header_ptr[0];

        cout << DEBUG_PREFIX << "Scanning page " << current_page_id << " with " << slot_count << " slots.\n";

        // Scan slots in current page
        while (current_slot_id < slot_count) {
            uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + current_slot_id * SLOT_SIZE]);
            uint16_t offset = slot_entry[0];
            uint16_t size = slot_entry[1];

            cout << DEBUG_PREFIX << "Checking slot " << current_slot_id << ": offset=" << offset << ", size=" << size << ".\n";

            if (offset != INVALID_SLOT && size > 0) {
                // Found valid record to yield next
                cout << DEBUG_PREFIX << "Found valid record at page " << current_page_id << ", slot " << current_slot_id << ".\n";
                return;
            }
            current_slot_id++;
        }

        // No valid slot found in current page, advance to next page
        cout << DEBUG_PREFIX << "No valid record found in page " << current_page_id << ". Moving to next page.\n";
        try {
            current_page_id++;
            page = disk.read_page(current_page_id);
            current_slot_id = 0;
        } catch (...) {
            cout << DEBUG_PREFIX << "No more pages available after page " << current_page_id - 1 << ".\n";
            current_page_id = -1; // mark iteration end
            return;
        }
    }
}

bool RecordIterator::has_next() const {
    cout << DEBUG_PREFIX << "has_next called. current_page_id=" << current_page_id << ".\n";
    return current_page_id >= 0;
}

Record RecordIterator::next() {
    if (!has_next()) {
        cout << DEBUG_PREFIX << "No more records available. Returning empty record.\n";
        return Record(vector<char>()); // Return empty record
    }

    uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
    uint16_t slot_count = header_ptr[0];

    if (current_slot_id >= slot_count) {
        cout << DEBUG_PREFIX << "No more records in the current page. Returning empty record.\n";
        return Record(vector<char>());
    }

    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + current_slot_id * SLOT_SIZE]);
    uint16_t offset = slot_entry[0];
    uint16_t size = slot_entry[1];

    if (offset == INVALID_SLOT || size == 0) {
        cout << DEBUG_PREFIX << "Invalid record at current slot. Returning empty record.\n";
        return Record(vector<char>());
    }

    vector<char> record_data(page.begin() + offset, page.begin() + offset + size);
    Record record(record_data);

    cout << DEBUG_PREFIX << "Returning record from page " << current_page_id << ", slot " << current_slot_id << ".\n";

    current_slot_id++;
    load_next_valid_record();

    return record;
}

std::tuple<Record, int, int> RecordIterator::next_with_location() {
    while (true) {
        if (!has_next()) {
            cout << DEBUG_PREFIX << "No more records available. Returning empty tuple.\n";
            return {Record(vector<char>()), -1, -1};
        }

        uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
        uint16_t slot_count = header_ptr[0];

        if (current_slot_id >= slot_count) {
            // No more slots on current page; try to load next page
            try {
                current_page_id++;
                page = disk.read_page(current_page_id);
                current_slot_id = 0;
            } catch (...) {
                cout << DEBUG_PREFIX << "No more pages available after page " << current_page_id - 1 << ".\n";
                current_page_id = -1;
                continue;  // loop again and will return empty tuple on next iteration
            }
            continue;
        }

        uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + current_slot_id * SLOT_SIZE]);
        uint16_t offset = slot_entry[0];
        uint16_t size = slot_entry[1];

        int slot_id = current_slot_id;
        int page_id = current_page_id;
        current_slot_id++;

        if (offset == INVALID_SLOT || size == 0) {
            cout << DEBUG_PREFIX << "Invalid record at page " << page_id << ", slot " << slot_id << ". Skipping.\n";
            continue; // skip invalid slot
        }

        vector<char> record_data(page.begin() + offset, page.begin() + offset + size);
        RecordID rid(page_id, slot_id);
        Record rec(record_data, rid);

        cout << DEBUG_PREFIX << "Returning record from page " << page_id << ", slot " << slot_id << ".\n";

        return {rec, page_id, slot_id};
    }
}
