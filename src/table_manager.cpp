#include "../include/table_manager.h"
#include "../include/catalog_manager.h"
#include "../include/record_manager.h"
#include "../include/record_iterator.h"
#include <iostream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <numeric>  // <-- needed for accumulate

#define DEBUG_COLOR_RESET      "\033[0m"
#define DEBUG_COLOR_YELLOW     "\033[33m"
#define DEBUG_COLOR_CYAN       "\033[36m"
#define DEBUG_DEBUG_LABEL      DEBUG_COLOR_YELLOW"[DEBUG]" DEBUG_COLOR_RESET
#define DEBUG_TABLE_LABEL      DEBUG_COLOR_CYAN "[TABLE_MANAGER]" DEBUG_COLOR_RESET
#define DEBUG_TABLE_MANAGER(msg) std::cout << DEBUG_DEBUG_LABEL << DEBUG_TABLE_LABEL << " " << msg << std::endl;


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
    ss << table_name << "|"; // Prefix with table name
    for (size_t i = 0; i < values.size(); ++i) {
        ss << values[i];
        if (i < values.size() - 1) ss << "|";
    }

    Record record(ss.str());
    int record_id = record_mgr.insert_record(record);

    // Index insert (no change)
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
        std::vector<int> to_delete;

        RecordIterator iterator(record_mgr.get_disk());
        const std::string table_prefix = table_name + "|";
        const std::string schema_prefix = "SCHEMA|";

        while (iterator.has_next()) {
            auto [rec, page_id, slot_id] = iterator.next_with_location();
            std::string rec_str = rec.to_string();

            // Skip schema records
            if (rec_str.rfind(schema_prefix, 0) == 0) continue;
            // Only delete records for this table
            if (rec_str.rfind(table_prefix, 0) != 0) continue;

            RecordID rid(page_id, slot_id);
            int rid_encoded = rid.encode();
            to_delete.push_back(rid_encoded);
        }

        for (int rid_encoded : to_delete) {
            // Call the original single-record deletion
            delete_from(table_name, rid_encoded);
            deleted_count++;
        }

        DEBUG_TABLE_MANAGER("Deleted " << deleted_count << " records from table: " << table_name);
        return true;
    }

    // Delete single record (existing code)
    DEBUG_TABLE_MANAGER("Deleting single record with id: " << record_id);
    record_mgr.delete_record(record_id);
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
    const std::string schema_prefix = "SCHEMA|";
    const std::string table_prefix = table_name + "|";
    while (it.has_next()) {
        Record rec = it.next();
        std::string rec_str(rec.data.begin(), rec.data.end());
        // Skip schema records
        if (rec_str.rfind(schema_prefix, 0) == 0) continue;
        // Only include records for this table
        if (rec_str.rfind(table_prefix, 0) != 0) continue;
        // Remove table name prefix before returning
        rec_str = rec_str.substr(table_prefix.size());
        rec.data = std::vector<char>(rec_str.begin(), rec_str.end());
        records.push_back(rec);
    }
    DEBUG_TABLE_MANAGER("Scanned " << records.size() << " records from table: " << table_name);
    return records;
}

void TableManager::printTable(const std::string& tableName) {
    // Get table schema
    TableSchema schema = catalog.get_schema(tableName);
    if (schema.table_name.empty()) {
        std::cerr << "[ERROR] Table '" << tableName << "' does not exist." << std::endl;
        return;
    }

    // Get all records
    std::vector<Record> records = scan(tableName);

    // Compute column widths
    std::vector<size_t> colWidths(schema.columns.size(), 0);
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        colWidths[i] = schema.columns[i].length();  // Start with header length
    }

    // Update column widths based on record contents
    for (const auto& rec : records) {
        std::string recStr(rec.data.begin(), rec.data.end());
        std::vector<std::string> values;
        std::stringstream ss(recStr);
        std::string field;
        while (std::getline(ss, field, '|')) {
            values.push_back(field);
        }

        for (size_t i = 0; i < values.size() && i < colWidths.size(); ++i) {
            colWidths[i] = std::max(colWidths[i], values[i].length());
        }
    }

    // Print header row
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        std::cout << "| " << std::setw(colWidths[i]) << std::left << schema.columns[i] << " ";
    }
    std::cout << "|\n";

    // Print separator
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        std::cout << "|-" << std::setw(colWidths[i]) << std::setfill('-') << "-" << std::setfill(' ');
    }
    std::cout << "|\n";

    // Print records
    if (records.empty()) {
        std::cout << "| " << std::setw(std::accumulate(colWidths.begin(), colWidths.end(), 3 * colWidths.size())) 
                  << std::left << "No records found" << " |\n";
        return;
    }

    for (const auto& rec : records) {
        std::string recStr(rec.data.begin(), rec.data.end());
        std::vector<std::string> values;
        std::stringstream ss(recStr);
        std::string field;
        while (std::getline(ss, field, '|')) {
            values.push_back(field);
        }

        for (size_t i = 0; i < schema.columns.size(); ++i) {
            std::string val = (i < values.size()) ? values[i] : "";
            std::cout << "| " << std::setw(colWidths[i]) << std::left << val << " ";
        }
        std::cout << "|\n";
    }
}
