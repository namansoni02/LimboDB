#include "./include/disk_manager.h"
#include "./include/record_manager.h"
#include <iostream>

int main() {
    DiskManager dm("dbfile.db");
    RecordManager rm(dm);

    // Insert records
    int id1 = rm.insert_record(Record("Hello World!"));
    int id2 = rm.insert_record(Record("Another Record"));

    // Print inserted records
    std::cout << "Record 1 ID: " << id1 << "\n";
    std::cout << "Record 2 ID: " << id2 << "\n";

    std::cout << "Record 1 Content: " << rm.get_record(id1).to_string() << "\n";
    std::cout << "Record 2 Content: " << rm.get_record(id2).to_string() << "\n";

    // Delete the first record
    rm.delete_record(id1);
    std::cout << "Record 1 deleted.\n";

    // Try to fetch deleted record
    try {
        auto r = rm.get_record(id1);
        std::cout << "Record 1 still exists: " << r.to_string() << "\n";
    } catch (const std::exception& e) {
        std::cout << "Failed to fetch Record 1: " << e.what() << "\n";
    }

    return 0;
}
