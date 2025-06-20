#include "../include/record_iterator.h"
#include <iostream>

using namespace std;

#define DEBUG_PREFIX "[DEBUG][RECORD_ITERATOR] "
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_RESET  "\033[0m"

// Remove 'valid' member and all logic related to it

RecordIterator::RecordIterator(DiskManager& disk_manager) 
    : disk(disk_manager), current_page_id(0), current_slot_id(0) {
    try {
        page = disk.read_page(current_page_id);
        cout << COLOR_GREEN << DEBUG_PREFIX << "Initialized at page " << current_page_id << "." << COLOR_RESET << endl;
        load_next_valid_record();
    } catch (...) {
        cout << COLOR_RED << DEBUG_PREFIX << "No pages available at initialization." << COLOR_RESET << endl;
        current_page_id = -1; // No valid page => end iterator
    }
}

void RecordIterator::load_next_valid_record() {
    while (current_page_id >= 0) {
        uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
        uint16_t slot_count = header_ptr[0];

        cout << COLOR_GREEN << DEBUG_PREFIX << "Scanning page " << current_page_id << " with " << slot_count << " slots." << COLOR_RESET << endl;

        // Scan slots in current page
        while (current_slot_id < slot_count) {
            uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + current_slot_id * SLOT_SIZE]);
            uint16_t offset = slot_entry[0];
            uint16_t size = slot_entry[1];

            cout << COLOR_GREEN << DEBUG_PREFIX << "Checking slot " << current_slot_id << ": offset=" << offset << ", size=" << size << "." << COLOR_RESET << endl;

            if (offset != INVALID_SLOT && size > 0) {
                // Found valid record to yield next
                cout << COLOR_GREEN << DEBUG_PREFIX << "Found valid record at page " << current_page_id << ", slot " << current_slot_id << "." << COLOR_RESET << endl;
                return;
            }
            current_slot_id++;
        }

        // No valid slot found in current page, advance to next page
        cout << COLOR_RED << DEBUG_PREFIX << "No valid record found in page " << current_page_id << ". Moving to next page." << COLOR_RESET << endl;
        try {
            current_page_id++;
            page = disk.read_page(current_page_id);
            current_slot_id = 0;
        } catch (...) {
            cout << COLOR_RED << DEBUG_PREFIX << "No more pages available after page " << current_page_id - 1 << "." << COLOR_RESET << endl;
            current_page_id = -1; // mark iteration end
            return;
        }
    }
}

bool RecordIterator::has_next() const {
    cout << COLOR_GREEN << DEBUG_PREFIX << "has_next called. current_page_id=" << current_page_id << "." << COLOR_RESET << endl;
    return current_page_id >= 0;
}

Record RecordIterator::next() {
    if (!has_next()) {
        cout << COLOR_RED << DEBUG_PREFIX << "No more records available. Returning empty record." << COLOR_RESET << endl;
        return Record(vector<char>()); // Return empty record
    }

    uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
    uint16_t slot_count = header_ptr[0];

    if (current_slot_id >= slot_count) {
        cout << COLOR_RED << DEBUG_PREFIX << "No more records in the current page. Returning empty record." << COLOR_RESET << endl;
        return Record(vector<char>());
    }

    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + current_slot_id * SLOT_SIZE]);
    uint16_t offset = slot_entry[0];
    uint16_t size = slot_entry[1];

    if (offset == INVALID_SLOT || size == 0) {
        cout << COLOR_RED << DEBUG_PREFIX << "Invalid record at current slot. Returning empty record." << COLOR_RESET << endl;
        return Record(vector<char>());
    }

    vector<char> record_data(page.begin() + offset, page.begin() + offset + size);
    Record record(record_data);

    cout << COLOR_GREEN << DEBUG_PREFIX << "Returning record from page " << current_page_id << ", slot " << current_slot_id << "." << COLOR_RESET << endl;

    current_slot_id++;
    load_next_valid_record();

    return record;
}

std::tuple<Record, int, int> RecordIterator::next_with_location() {
    while (true) {
        if (!has_next()) {
            cout << COLOR_RED << DEBUG_PREFIX << "No more records available. Returning empty tuple." << COLOR_RESET << endl;
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
                cout << COLOR_RED << DEBUG_PREFIX << "No more pages available after page " << current_page_id - 1 << "." << COLOR_RESET << endl;
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
            cout << COLOR_RED << DEBUG_PREFIX << "Invalid record at page " << page_id << ", slot " << slot_id << ". Skipping." << COLOR_RESET << endl;
            continue; // skip invalid slot
        }

        vector<char> record_data(page.begin() + offset, page.begin() + offset + size);
        RecordID rid(page_id, slot_id);
        Record rec(record_data, rid);

        cout << COLOR_GREEN << DEBUG_PREFIX << "Returning record from page " << page_id << ", slot " << slot_id << "." << COLOR_RESET << endl;

        return {rec, page_id, slot_id};
    }
}
