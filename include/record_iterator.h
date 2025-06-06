#include<iostream>
#include"disk_manager.h"
#include"record_manager.h"
#include <tuple>

using namespace std;

class RecordIterator {
private:
    DiskManager& disk;
    int current_page_id;
    int current_slot_id;
    std::vector<char> page;

    void load_next_valid_record();

public:
    RecordIterator(DiskManager& disk_manager);

    bool has_next() const;

    Record next();
    tuple<Record, int, int> next_with_location();
};