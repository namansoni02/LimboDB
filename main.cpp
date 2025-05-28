#include <iostream>
#include "./include/disk_manager.h"
#include "./include/record_manager.h"
#include "./include/record_iterator.h"

using namespace std;

int main() {
    cout << "Initializing disk and record manager..." << endl;

    DiskManager disk("dbfile.db");
    RecordManager rm(disk);

    // Insert a few records
    cout << "\nInserting records..." << endl;
    vector<int> record_ids;
    for (int i = 0; i < 5; ++i) {
        string data = "Record #" + to_string(i);
        Record record(vector<char>(data.begin(), data.end()));
        int id = rm.insert_record(record);
        record_ids.push_back(id);
        cout << "Inserted: " << data << " with id " << id << endl;
    }

    // Read and print them back
    cout << "\nReading inserted records..." << endl;
    for (int id : record_ids) {
        Record r = rm.get_record(id);
        string data(r.data.begin(), r.data.end());
        cout << "Record ID " << id << ": " << data << endl;
    }

    // Delete one record
    cout << "\nDeleting record id " << record_ids[2] << endl;
    rm.delete_record(record_ids[2]);

    // Iterate through all valid records
    cout << "\nIterating records with RecordIterator..." << endl;
    RecordIterator it(disk);
    while (it.has_next()) {
        Record r = it.next();
        string data(r.data.begin(), r.data.end());
        cout << "Iterated Record: " << data << endl;
    }

    return 0;
}
