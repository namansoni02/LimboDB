#include"../include/record_iterator.h"
// #include "../constants.h"
#include<iostream>

using namespace std;

RecordIterator::RecordIterator(DiskManager& disk_manager) 
    : disk(disk_manager), current_page_id(0), current_slot_id(0) {
    try{
        page = disk.read_page(current_page_id);
        load_next_valid_record();
    }catch(...){
        cout<< "[DEBUG] No pages available at initialization.\n";
        current_page_id = -1; // No valid page
    }
}

void RecordIterator::load_next_valid_record() {
    while (true) {
        if (current_page_id < 0) {
            std::cout << "[DEBUG] End of pages reached.\n";
            return;
        }

        uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
        uint16_t slot_count = header_ptr[0];

        if (slot_count == 0) {
            // No slots means probably empty page, check if next page exists
            try {
                current_page_id++;
                page = disk.read_page(current_page_id);
                current_slot_id = 0;
                continue; // Keep scanning next page
            } catch (...) {
                // No more pages
                std::cout << "[DEBUG] No more pages available.\n";
                current_page_id = -1;
                return;
            }
        }

        // existing loop for slots
        while (current_slot_id < slot_count) {
            uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + current_slot_id * SLOT_SIZE]);
            uint16_t offset = slot_entry[0];
            uint16_t size = slot_entry[1];

            if (offset != INVALID_SLOT && size > 0) {
                return; // found valid record
            }
            current_slot_id++;
        }

        // no valid record found in this page, move to next page
        try {
            current_page_id++;
            page = disk.read_page(current_page_id);
            current_slot_id = 0;
        } catch (...) {
            std::cout << "[DEBUG] No more pages available.\n";
            current_page_id = -1;
            return;
        }
    }
}


bool RecordIterator::has_next() const {
    return current_page_id >= 0;
}

Record RecordIterator::next() {
    if (!has_next()) {
        throw std::out_of_range("No more records available.");
    }

    uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
    uint16_t slot_count = header_ptr[0];

    if (current_slot_id >= slot_count) {
        throw std::out_of_range("No more records in the current page.");
    }

    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + current_slot_id * SLOT_SIZE]);
    uint16_t offset = slot_entry[0];
    uint16_t size = slot_entry[1];

    if (offset == INVALID_SLOT || size == 0) {
        throw std::runtime_error("Invalid record at current slot.");
    }

    vector<char> record_data(page.begin() + offset, page.begin() + offset + size);
    Record record(record_data);

    cout << "[DEBUG] Returning record from page " << current_page_id << ", slot " << current_slot_id << "\n";

    // Move to next slot
    current_slot_id++;
    load_next_valid_record();

    return record;
}