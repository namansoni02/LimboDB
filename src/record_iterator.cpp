#include "../include/record_iterator.h"
// #include "../constants.h"
#include <iostream>

using namespace std;

#define DEBUG_PREFIX "[DEBUG][RECORD_ITERATOR] "

RecordIterator::RecordIterator(DiskManager& disk_manager) 
    : disk(disk_manager), current_page_id(0), current_slot_id(0) {
    try {
        page = disk.read_page(current_page_id);
        cout << DEBUG_PREFIX << "Initialized at page " << current_page_id << ".\n";
        load_next_valid_record();
    } catch (...) {
        cout << DEBUG_PREFIX << "No pages available at initialization.\n";
        current_page_id = -1; // No valid page
    }
}

void RecordIterator::load_next_valid_record() {
    while (true) {
        if (current_page_id < 0) {
            cout << DEBUG_PREFIX << "End of pages reached.\n";
            return;
        }

        uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
        uint16_t slot_count = header_ptr[0];

        cout << DEBUG_PREFIX << "Scanning page " << current_page_id << " with " << slot_count << " slots.\n";

        if (slot_count == 0) {
            cout << DEBUG_PREFIX << "Page " << current_page_id << " is empty. Moving to next page.\n";
            try {
                current_page_id++;
                page = disk.read_page(current_page_id);
                current_slot_id = 0;
                continue;
            } catch (...) {
                cout << DEBUG_PREFIX << "No more pages available after page " << current_page_id - 1 << ".\n";
                current_page_id = -1;
                return;
            }
        }

        while (current_slot_id < slot_count) {
            uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + current_slot_id * SLOT_SIZE]);
            uint16_t offset = slot_entry[0];
            uint16_t size = slot_entry[1];

            cout << DEBUG_PREFIX << "Checking slot " << current_slot_id << ": offset=" << offset << ", size=" << size << ".\n";

            if (offset != INVALID_SLOT && size > 0) {
                cout << DEBUG_PREFIX << "Found valid record at page " << current_page_id << ", slot " << current_slot_id << ".\n";
                return;
            }
            current_slot_id++;
        }

        cout << DEBUG_PREFIX << "No valid record found in page " << current_page_id << ". Moving to next page.\n";
        try {
            current_page_id++;
            page = disk.read_page(current_page_id);
            current_slot_id = 0;
        } catch (...) {
            cout << DEBUG_PREFIX << "No more pages available after page " << current_page_id - 1 << ".\n";
            current_page_id = -1;
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
        cout << DEBUG_PREFIX << "No more records available. Throwing exception.\n";
        throw std::out_of_range("No more records available.");
    }

    uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
    uint16_t slot_count = header_ptr[0];

    if (current_slot_id >= slot_count) {
        cout << DEBUG_PREFIX << "No more records in the current page. Throwing exception.\n";
        throw std::out_of_range("No more records in the current page.");
    }

    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + current_slot_id * SLOT_SIZE]);
    uint16_t offset = slot_entry[0];
    uint16_t size = slot_entry[1];

    if (offset == INVALID_SLOT || size == 0) {
        cout << DEBUG_PREFIX << "Invalid record at current slot. Throwing exception.\n";
        throw std::runtime_error("Invalid record at current slot.");
    }

    vector<char> record_data(page.begin() + offset, page.begin() + offset + size);
    Record record(record_data);

    cout << DEBUG_PREFIX << "Returning record from page " << current_page_id << ", slot " << current_slot_id << ".\n";

    // Move to next slot
    current_slot_id++;
    load_next_valid_record();

    return record;
}

tuple<Record, int, int> RecordIterator::next_with_location() {
    while(current_page_id < disk.get_num_pages()){
        vector<char> page = disk.read_page(current_page_id);
        while(current_slot_id < page.size() / SLOT_SIZE){
            if(!)
        }
    }
}