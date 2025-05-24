#include "../include/record_manager.h"
#include <iostream>
#include <iomanip> // for std::hex and std::setw

RecordManager::RecordManager(DiskManager& dm) : disk(dm), next_page_id(0) {
    std::cout << "[DEBUG] RecordManager initialized." << std::endl;
}

std::pair<int, int> RecordManager::decode_record_id(int record_id) {
    int page_id = record_id >> 16;
    int slot_id = record_id & 0xFFFF;
    std::cout << "[DEBUG] Decoded record_id " << record_id << " to page_id " << page_id << " and slot_id " << slot_id << std::endl;
    return {page_id, slot_id};
}

int RecordManager::encode_record_id(int page_id, int slot_id) {
    int record_id = (page_id << 16) | slot_id;
    std::cout << "[DEBUG] Encoded page_id " << page_id << " and slot_id " << slot_id << " to record_id " << record_id << std::endl;
    return record_id;
}

int RecordManager::find_free_page() {
    int page_id = 0;
    std::cout << "[DEBUG] Searching for free page starting at page_id = 0" << std::endl;
    while (true) {
        std::vector<char> page;
        try {
            page = disk.read_page(page_id);
            std::cout << "[DEBUG] Read page " << page_id << std::endl;
        } catch (...) {
            std::cout << "[DEBUG] Page " << page_id << " does not exist yet. Using this page." << std::endl;
            return page_id; // page does not exist yet
        }

        uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
        uint16_t slot_count = header_ptr[0];
        uint16_t free_offset = header_ptr[1];

        std::cout << "[DEBUG] Page " << page_id << " header: slot_count=" << slot_count << ", free_offset=" << free_offset << std::endl;

        if (slot_count == 0 && free_offset == 0) {
            std::cout << "[DEBUG] Initializing header for page " << page_id << std::endl;
            slot_count = 0;
            free_offset = PAGE_SIZE;
            memcpy(&page[0], &slot_count, sizeof(uint16_t));
            memcpy(&page[2], &free_offset, sizeof(uint16_t));

            bool success = disk.write_page(page_id, page);
            if (!success) {
                std::cerr << "[ERROR] Failed to write page " << page_id << std::endl;
                throw std::runtime_error("Failed to write page");
            }
            std::cout << "[DEBUG] Writing page " << page_id << std::endl;
            return page_id;
        }

        int available = free_offset - (HEADER_SIZE + slot_count * SLOT_SIZE);
        std::cout << "[DEBUG] Page " << page_id << " available space for record + slot: " << available << std::endl;

        if (available >= 4 + SLOT_SIZE) { // minimum record size + slot size
            std::cout << "[DEBUG] Page " << page_id << " has enough space. Using this page." << std::endl;
            return page_id;
        }

        page_id++;
        std::cout << "[DEBUG] Moving to next page: " << page_id << std::endl;
    }
}

int RecordManager::insert_record(const Record& record) {
    std::cout << "[DEBUG] Inserting record: " << record.to_string() << std::endl;
    int page_id = find_free_page();
    std::vector<char> page;

    try {
        page = disk.read_page(page_id);
        std::cout << "[DEBUG] Page " << page_id << " read successfully." << std::endl;
    } catch (...) {
        std::cout << "[DEBUG] Page " << page_id << " does not exist. Initializing new." << std::endl;
        page = std::vector<char>(PAGE_SIZE, 0);
        uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
        header_ptr[0] = 0;          // slot_count
        header_ptr[1] = PAGE_SIZE;  // free_offset

        std::cout << "[DEBUG] Initialized header bytes: ";
        for (int i = 0; i < 4; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (static_cast<uint16_t>(page[i]) & 0xFF) << " ";
        }
        std::cout << std::dec << std::endl;
    }

    uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
    uint16_t slot_count = header_ptr[0];
    uint16_t free_offset = header_ptr[1];

    std::cout << "[DEBUG] Current slot_count: " << slot_count << ", free_offset: " << free_offset << std::endl;

    uint16_t rec_size = static_cast<uint16_t>(record.data.size());
    std::cout << "[DEBUG] Record size: " << rec_size << std::endl;

    int available = free_offset - (HEADER_SIZE + slot_count * SLOT_SIZE);
    if (available < rec_size + SLOT_SIZE) {
        std::cerr << "[ERROR] Not enough space in page " << page_id << " for record size " << rec_size << std::endl;
        throw std::runtime_error("Page does not have enough space");
    }

    free_offset -= rec_size;
    std::cout << "[DEBUG] Updated free_offset after inserting record data: " << free_offset << std::endl;

    // Copy record data
    memcpy(&page[free_offset], record.data.data(), rec_size);
    std::cout << "[DEBUG] Record data copied to page at offset " << free_offset << std::endl;

    // Write slot entry
    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + slot_count * SLOT_SIZE]);
    slot_entry[0] = free_offset;
    slot_entry[1] = rec_size;
    std::cout << "[DEBUG] Slot entry written at index " << slot_count << ": offset=" << free_offset << ", size=" << rec_size << std::endl;

    // Update header
    slot_count++;
    header_ptr[0] = slot_count;
    header_ptr[1] = free_offset;
    std::cout << "[DEBUG] Header updated: slot_count=" << slot_count << ", free_offset=" << free_offset << std::endl;

    bool success = disk.write_page(page_id, page);
    if (!success) {
        std::cerr << "[ERROR] Failed to write page " << page_id << " to disk." << std::endl;
        throw std::runtime_error("Failed to write page");
    }
    std::cout << "[DEBUG] Page " << page_id << " written to disk." << std::endl;

    std::cout << "[DEBUG] Record inserted at page " << page_id << " slot " << (slot_count - 1) << std::endl;

    return encode_record_id(page_id, slot_count - 1);
}

Record RecordManager::get_record(int record_id) {
    auto [page_id, slot_id] = decode_record_id(record_id);
    std::cout << "[DEBUG] Getting record at page " << page_id << ", slot " << slot_id << std::endl;

    std::vector<char> page = disk.read_page(page_id);
    std::cout << "[DEBUG] Page " << page_id << " read from disk." << std::endl;

    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + slot_id * SLOT_SIZE]);
    uint16_t offset = slot_entry[0];
    uint16_t size = slot_entry[1];

    std::cout << "[DEBUG] Slot entry: offset=" << offset << ", size=" << size << std::endl;

    if (offset == INVALID_SLOT || size == 0) {
        std::cerr << "[ERROR] Record not found or deleted at page " << page_id << ", slot " << slot_id << std::endl;
        throw std::runtime_error("Record not found or deleted");
    }

    std::cout << "[DEBUG] Record data retrieved successfully." << std::endl;
    return Record(std::vector<char>(page.begin() + offset, page.begin() + offset + size));
}

void RecordManager::delete_record(int record_id) {
    auto [page_id, slot_id] = decode_record_id(record_id);
    std::cout << "[DEBUG] Deleting record at page " << page_id << ", slot " << slot_id << std::endl;

    std::vector<char> page = disk.read_page(page_id);
    std::cout << "[DEBUG] Page " << page_id << " read from disk for deletion." << std::endl;

    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + slot_id * SLOT_SIZE]);
    slot_entry[0] = INVALID_SLOT;
    slot_entry[1] = 0;

    std::cout << "[DEBUG] Slot entry marked as invalid." << std::endl;

    bool success = disk.write_page(page_id, page);
    if (!success) {
        std::cerr << "[ERROR] Failed to write page " << page_id << " after deletion." << std::endl;
        throw std::runtime_error("Failed to write page");
    }
    std::cout << "[DEBUG] Page " << page_id << " written after deletion." << std::endl;
}
