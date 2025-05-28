#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <cstdint>

// Page structure
const int PAGE_SIZE = 4096;       // Size of each page in bytes
const int HEADER_SIZE = 4;        // 2 bytes for slot_count + 2 bytes for free_offset
const int SLOT_SIZE = 4;          // 2 bytes for offset + 2 bytes for length

// Special values
const uint16_t INVALID_SLOT = 0xFFFF;  // Used to mark deleted records

#endif // CONSTANTS_H
