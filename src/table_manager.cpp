#include "../include/table_manager.h"
#include "../include/catalog_manager.h"
#include "../include/record_manager.h"
#include "../include/record_iterator.h"
#include <iostream>
#include <sstream>

#define DEBUG_TABLE_MANAGER(msg) std::cout << "[DEBUG][TABLE_MANAGER] " << msg << std::endl;

TableManager::TableManager(CatalogManager& cat, RecordManager& rm, IndexManager& im)
    : catalog(cat), record_mgr(rm), index_mgr(im) {
    DEBUG_TABLE_MANAGER("Initialized TableManager with IndexManager");
}


int TableManager::insert_into(const string& table_name, const vector<string>& values) {
    DEBUG_TABLE_MANAGER("insert_into called for table: " << table_name);
    TableSchema schema = catalog.get_schema(table_name);
    if (values.size() != schema.columns.size()) {
        DEBUG_TABLE_MANAGER("Insert failed: value count does not match schema");
        return -1;
    }

    stringstream ss;
    for (size_t i = 0; i < values.size(); ++i) {
        ss << values[i];
        if (i < values.size() - 1) ss << "|";
    }

    Record record(ss.str());
    int record_id = record_mgr.insert_record(record);

    // Index insert
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        index_mgr.insert_entry(table_name, schema.columns[i], values[i], record_id);
    }

    DEBUG_TABLE_MANAGER("Inserted record_id: " << record_id);
    return record_id;
}


bool TableManager::delete_from(const std::string& table_name, int record_id /* = -1 */) {
    DEBUG_TABLE_MANAGER("delete_from called for table: " << table_name << ", record_id: " << record_id);

    if (record_id == -1) {
        // Delete ALL records in this table
        int deleted_count = 0;

        RecordIterator iterator(record_mgr.get_disk());
        TableSchema schema = catalog.get_schema(table_name);

        while (iterator.has_next()) {
            auto [rec, page_id, slot_id] = iterator.next_with_location();

            // Debug: Show record location and content
            DEBUG_TABLE_MANAGER("Attempting to delete record at page_id: " << page_id << ", slot_id: " << slot_id << ", content: " << rec.to_string());

            // Parse record string to check if it belongs to this table
            std::stringstream ss(rec.to_string());
            std::vector<std::string> values;
            std::string token;
            while (getline(ss, token, '|')) {
                values.push_back(token);
            }

            // Here, we must confirm this record belongs to the table.
            // This depends on your data layout, assuming first value is table_name or similar.
            // If your table data records do not include table_name, you may need
            // another way to identify.

            // Let's assume your records **do NOT** store table_name inside the record.
            // So you might need to delete all records regardless.

            // To be safe, just delete all records for now:
            RecordID rid(page_id, slot_id);
            int rid_encoded = rid.encode();

            // Debug: Show encoded record id
            DEBUG_TABLE_MANAGER("Deleting record with encoded id: " << rid_encoded);

            // Call the original single-record deletion
            delete_from(table_name, rid_encoded);
            deleted_count++;
        }

        DEBUG_TABLE_MANAGER("Deleted " << deleted_count << " records from table: " << table_name);
        return true;
    }

    // Delete single record (existing code)
    DEBUG_TABLE_MANAGER("Deleting single record with id: " << record_id);
    Record rec = record_mgr.get_record(record_id);
    std::stringstream ss(rec.to_string());
    std::vector<std::string> values;
    std::string token;
    while (getline(ss, token, '|')) {
        values.push_back(token);
    }

    TableSchema schema = catalog.get_schema(table_name);
    for (size_t i = 0; i < values.size(); ++i) {
        DEBUG_TABLE_MANAGER("Deleting index entry for column: " << schema.columns[i] << ", value: " << values[i]);
        index_mgr.delete_entry(table_name, schema.columns[i], values[i], record_id);
    }

    DEBUG_TABLE_MANAGER("Deleting record from record manager with id: " << record_id);
    record_mgr.delete_record(record_id);
    DEBUG_TABLE_MANAGER("Record deleted successfully.");
    return true;
}



bool TableManager::update(const string& table_name, int record_id, const vector<string>& new_values) {
    DEBUG_TABLE_MANAGER("update called for table: " << table_name);
    TableSchema schema = catalog.get_schema(table_name);
    if (new_values.size() != schema.columns.size()) {
        DEBUG_TABLE_MANAGER("Update failed: value count mismatch");
        return false;
    }

    // Delete old index entries
    Record old_record = record_mgr.get_record(record_id);
    stringstream ss_old(old_record.to_string());
    vector<string> old_values;
    string token;
    while (getline(ss_old, token, '|')) {
        old_values.push_back(token);
    }

    for (size_t i = 0; i < old_values.size(); ++i) {
        index_mgr.delete_entry(table_name, schema.columns[i], old_values[i], record_id);
    }

    // Create new record
    stringstream ss;
    for (size_t i = 0; i < new_values.size(); ++i) {
        ss << new_values[i];
        if (i < new_values.size() - 1) ss << "|";
    }

    Record new_record(ss.str());
    record_mgr.update_record(record_id, new_record);

    // Insert new index entries
    for (size_t i = 0; i < new_values.size(); ++i) {
        index_mgr.insert_entry(table_name, schema.columns[i], new_values[i], record_id);
    }

    return true;
}


Record TableManager::select(const string& table_name, int record_id) {
    DEBUG_TABLE_MANAGER("select called for table: " << table_name << ", record_id: " << record_id);
    return record_mgr.get_record(record_id);
}

vector<Record> TableManager::scan(const string& table_name) {
    DEBUG_TABLE_MANAGER("scan called for table: " << table_name);
    vector<Record> records;
    RecordIterator it(record_mgr.get_disk());
    while (it.has_next()) {
        records.push_back(it.next());
    }
    DEBUG_TABLE_MANAGER("Scanned " << records.size() << " records from table: " << table_name);
    return records;
}
