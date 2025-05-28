#include "../include/record_manager.h"
#include <iostream>
#include <iomanip> // for std::hex and std::setw

#define RM_DEBUG_PREFIX "[DEBUG][RECORD_MANAGER] "

RecordManager::RecordManager(DiskManager& dm) : disk(dm), next_page_id(0) {
    std::cout << RM_DEBUG_PREFIX << "RecordManager initialized." << std::endl;
}

std::pair<int, int> RecordManager::decode_record_id(int record_id) {
    int page_id = record_id >> 16;
    int slot_id = record_id & 0xFFFF;
    std::cout << RM_DEBUG_PREFIX << "Decoded record_id " << record_id << " to page_id " << page_id << " and slot_id " << slot_id << std::endl;
    return {page_id, slot_id};
}

int RecordManager::encode_record_id(int page_id, int slot_id) {
    int record_id = (page_id << 16) | slot_id;
    std::cout << RM_DEBUG_PREFIX << "Encoded page_id " << page_id << " and slot_id " << slot_id << " to record_id " << record_id << std::endl;
    return record_id;
}

int RecordManager::find_free_page() {
    int page_id = 0;
    std::cout << RM_DEBUG_PREFIX << "Searching for free page starting at page_id = 0" << std::endl;
    while (true) {
        std::vector<char> page;
        bool page_exists = true;

        try {
            page = disk.read_page(page_id);
            std::cout << RM_DEBUG_PREFIX << "Read page " << page_id << std::endl;
        } catch (...) {
            std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " does not exist yet. Allocating new page." << std::endl;
            page_id = disk.allocate_page();
            page = vector<char>(PAGE_SIZE, 0);
            page_exists = false;
        }

        uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
        uint16_t slot_count = header_ptr[0];
        uint16_t free_offset = header_ptr[1];

        if (!page_exists) {
            std::cout << RM_DEBUG_PREFIX << "Initializing header for new page " << page_id << std::endl;
            slot_count = 0;
            free_offset = PAGE_SIZE;
            header_ptr[0] = slot_count;
            header_ptr[1] = free_offset;
            bool success = disk.write_page(page_id, page);
            if (!success) {
                std::cerr << "[ERROR][RECORD_MANAGER] Failed to write page " << page_id << std::endl;
                throw std::runtime_error("Failed to write page");
            }
            return page_id;
        }

        int available = free_offset - (HEADER_SIZE + slot_count * SLOT_SIZE);
        std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " available space for record + slot: " << available << std::endl;

        if (available >= 4 + SLOT_SIZE) { // minimum record size + slot size
            std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " has enough space. Using this page." << std::endl;
            return page_id;
        }

        page_id++;
        std::cout << RM_DEBUG_PREFIX << "Moving to next page: " << page_id << std::endl;
    }
}

int RecordManager::insert_record(const Record& record) {
    std::cout << RM_DEBUG_PREFIX << "Inserting record: " << record.to_string() << std::endl;
    int page_id = find_free_page();
    std::vector<char> page;

    try {
        page = disk.read_page(page_id);
        std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " read successfully." << std::endl;
    } catch (...) {
        std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " does not exist. Initializing new." << std::endl;
        page = std::vector<char>(PAGE_SIZE, 0);
        uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
        header_ptr[0] = 0;          // slot_count
        header_ptr[1] = PAGE_SIZE;  // free_offset
    }

    uint16_t* header_ptr = reinterpret_cast<uint16_t*>(page.data());
    uint16_t slot_count = header_ptr[0];
    uint16_t free_offset = header_ptr[1];

    std::cout << RM_DEBUG_PREFIX << "Current slot_count: " << slot_count << ", free_offset: " << free_offset << std::endl;

    uint16_t rec_size = static_cast<uint16_t>(record.data.size());
    std::cout << RM_DEBUG_PREFIX << "Record size: " << rec_size << std::endl;

    int available = free_offset - (HEADER_SIZE + slot_count * SLOT_SIZE);
    if (available < rec_size + SLOT_SIZE) {
        std::cerr << "[ERROR][RECORD_MANAGER] Not enough space in page " << page_id << " for record size " << rec_size << std::endl;
        throw std::runtime_error("Page does not have enough space");
    }

    free_offset -= rec_size;
    std::cout << RM_DEBUG_PREFIX << "Updated free_offset after inserting record data: " << free_offset << std::endl;

    // Copy record data
    memcpy(&page[free_offset], record.data.data(), rec_size);
    std::cout << RM_DEBUG_PREFIX << "Record data copied to page at offset " << free_offset << std::endl;

    // Write slot entry
    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + slot_count * SLOT_SIZE]);
    slot_entry[0] = free_offset;
    slot_entry[1] = rec_size;
    std::cout << RM_DEBUG_PREFIX << "Slot entry written at index " << slot_count << ": offset=" << free_offset << ", size=" << rec_size << std::endl;

    // Update header
    slot_count++;
    header_ptr[0] = slot_count;
    header_ptr[1] = free_offset;
    std::cout << RM_DEBUG_PREFIX << "Header updated: slot_count=" << slot_count << ", free_offset=" << free_offset << std::endl;

    bool success = disk.write_page(page_id, page);
    if (!success) {
        std::cerr << "[ERROR][RECORD_MANAGER] Failed to write page " << page_id << " to disk." << std::endl;
        throw std::runtime_error("Failed to write page");
    }
    std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " written to disk." << std::endl;

    std::cout << RM_DEBUG_PREFIX << "Record inserted at page " << page_id << " slot " << (slot_count - 1) << std::endl;

    return encode_record_id(page_id, slot_count - 1);
}

Record RecordManager::get_record(int record_id) {
    auto [page_id, slot_id] = decode_record_id(record_id);
    std::cout << RM_DEBUG_PREFIX << "Getting record at page " << page_id << ", slot " << slot_id << std::endl;

    std::vector<char> page;
    try {
        page = disk.read_page(page_id);
        std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " read from disk." << std::endl;
    } catch (...) {
        std::cerr << "[ERROR][RECORD_MANAGER] Failed to read page " << page_id << std::endl;
        throw std::runtime_error("Page read error");
    }

    if (page.size() < HEADER_SIZE + (slot_id + 1) * SLOT_SIZE) {
        std::cerr << "[ERROR][RECORD_MANAGER] Slot ID " << slot_id << " out of bounds in page " << page_id << std::endl;
        throw std::runtime_error("Invalid slot ID");
    }

    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + slot_id * SLOT_SIZE]);
    uint16_t offset = slot_entry[0];
    uint16_t size = slot_entry[1];

    std::cout << RM_DEBUG_PREFIX << "Slot entry: offset=" << offset << ", size=" << size << std::endl;

    if (offset == INVALID_SLOT || size == 0 || offset + size > PAGE_SIZE) {
        std::cerr << "[ERROR][RECORD_MANAGER] Record not found or invalid range at page " << page_id << ", slot " << slot_id << std::endl;
        throw std::runtime_error("Record not found or invalid range");
    }

    std::vector<char> record_data(page.begin() + offset, page.begin() + offset + size);
    std::cout << RM_DEBUG_PREFIX << "Record data retrieved successfully." << std::endl;
    return Record(record_data);
}

void RecordManager::delete_record(int record_id) {
    auto [page_id, slot_id] = decode_record_id(record_id);
    std::cout << RM_DEBUG_PREFIX << "Deleting record at page " << page_id << ", slot " << slot_id << std::endl;

    std::vector<char> page;
    try {
        page = disk.read_page(page_id);
        std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " read from disk for deletion." << std::endl;
    } catch (...) {
        std::cerr << "[ERROR][RECORD_MANAGER] Failed to read page " << page_id << " for deletion." << std::endl;
        throw std::runtime_error("Page read error during deletion");
    }

    if (page.size() < HEADER_SIZE + (slot_id + 1) * SLOT_SIZE) {
        std::cerr << "[ERROR][RECORD_MANAGER] Slot ID " << slot_id << " out of bounds in page " << page_id << std::endl;
        throw std::runtime_error("Invalid slot ID for deletion");
    }

    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + slot_id * SLOT_SIZE]);
    uint16_t offset = slot_entry[0];
    uint16_t size = slot_entry[1];

    if (offset == INVALID_SLOT || size == 0) {
        std::cerr << "[WARNING][RECORD_MANAGER] Record at page " << page_id << ", slot " << slot_id << " is already deleted or invalid." << std::endl;
        // Optional: You could throw here or just log and return
    }

    slot_entry[0] = INVALID_SLOT;
    slot_entry[1] = 0;

    std::cout << RM_DEBUG_PREFIX << "Slot entry marked as invalid." << std::endl;

    bool success = disk.write_page(page_id, page);
    if (!success) {
        std::cerr << "[ERROR][RECORD_MANAGER] Failed to write page " << page_id << " after deletion." << std::endl;
        throw std::runtime_error("Failed to write page after deletion");
    }
    std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " written after deletion." << std::endl;
}

int RecordManager::update_record(int record_id, const Record& new_record) {
    auto [page_id, slot_id] = decode_record_id(record_id);
    std::cout << RM_DEBUG_PREFIX << "Updating record at page " << page_id << ", slot " << slot_id << std::endl;

    std::vector<char> page = disk.read_page(page_id);
    std::cout << RM_DEBUG_PREFIX << "Page " << page_id << " read from disk for update." << std::endl;

    uint16_t* slot_entry = reinterpret_cast<uint16_t*>(&page[HEADER_SIZE + slot_id * SLOT_SIZE]);
    uint16_t offset = slot_entry[0];
    uint16_t size = slot_entry[1];

    if (offset == INVALID_SLOT || size == 0) {
        std::cerr << "[ERROR][RECORD_MANAGER] Cannot update: Record not found or deleted." << std::endl;
        throw std::runtime_error("Record not found or deleted");
    }

    uint16_t new_size = static_cast<uint16_t>(new_record.data.size());

    if (new_size <= size) {
        // Overwrite in place
        memcpy(&page[offset], new_record.data.data(), new_size);
        slot_entry[1] = new_size;

        std::cout << RM_DEBUG_PREFIX << "Record updated in place. New size: " << new_size << std::endl;

        bool success = disk.write_page(page_id, page);
        if (!success) {
            std::cerr << "[ERROR][RECORD_MANAGER] Failed to write updated page." << std::endl;
            throw std::runtime_error("Failed to update record");
        }
        return record_id;
    } else {
        // Not enough space, delete old and insert new
        std::cout << RM_DEBUG_PREFIX << "New record too large. Re-inserting in new page." << std::endl;

        delete_record(record_id);
        return insert_record(new_record);  // new record_id returned
    }
}
