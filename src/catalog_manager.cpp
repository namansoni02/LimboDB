#include "../include/catalog_manager.h"
#include <iostream>
#include <sstream>
#include "../include/record_iterator.h"
#include "../include/table_manager.h"
#include <algorithm>

// ANSI color codes for debug output
#define COLOR_RESET   "\033[0m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"

#define DEBUG_CATALOG(msg) \
    std::cout << COLOR_YELLOW << "[DEBUG]" << COLOR_CYAN << "[CATALOG_MANAGER] " << COLOR_RESET << msg << std::endl;

// ---------- TableSchema Methods ----------

std::string TableSchema::serialize() const {
    std::ostringstream oss;
    oss << "SCHEMA|" << table_name << "|";
    for (size_t i = 0; i < columns.size(); ++i) {
        oss << columns[i];
        if (i + 1 < columns.size()) oss << ",";
    }
    DEBUG_CATALOG("Serialized schema for table '" << table_name << "': " << oss.str());
    return oss.str();
}

// Update deserialization to check prefix
TableSchema TableSchema::deserialize(const std::string& record_str) {
    const std::string prefix = "SCHEMA|";
    if (record_str.rfind(prefix, 0) != 0) { // Not a schema record
        DEBUG_CATALOG("Skipped non-schema record: '" << record_str << "'");
        return TableSchema{};
    }
    size_t sep = record_str.find('|', prefix.size());
    if (sep == std::string::npos) {
        DEBUG_CATALOG("Failed to deserialize: missing separator in '" << record_str << "'");
        return TableSchema{};
    }

    TableSchema schema;
    schema.table_name = record_str.substr(prefix.size(), sep - prefix.size());
    std::string cols = record_str.substr(sep + 1);

    size_t pos = 0, prev = 0;
    while ((pos = cols.find(',', prev)) != std::string::npos) {
        schema.columns.push_back(cols.substr(prev, pos - prev));
        prev = pos + 1;
    }
    if (prev < cols.size())
        schema.columns.push_back(cols.substr(prev));

    DEBUG_CATALOG("Deserialized schema for table '" << schema.table_name << "' with columns: " << cols);
    return schema;
}

// ---------- CatalogManager Methods ----------

CatalogManager::CatalogManager(RecordManager& rm, IndexManager& im)
    : record_manager(rm), index_manager(im) {
    DEBUG_CATALOG("Initializing CatalogManager");
    load_catalog();
}

void CatalogManager::load_catalog() {
    DEBUG_CATALOG("Loading catalog from disk");
    RecordIterator iter(record_manager.get_disk());
    int count = 0;

    while (iter.has_next()) {
        try {
            auto [rec, page_id, slot_id] = iter.next_with_location();
            TableSchema schema = TableSchema::deserialize(rec.to_string());
            if (!schema.table_name.empty()) {
                schema_cache[schema.table_name] = schema;
                ++count;
            }
        } catch (const std::exception& e) {
            DEBUG_CATALOG("Error loading schema: " << e.what());
        }
    }

    DEBUG_CATALOG("Loaded " << count << " table schemas into cache");
}

bool CatalogManager::create_table(const std::string& table_name, const std::vector<std::string>& columns) {
    DEBUG_CATALOG("Attempting to create table '" << table_name << "'");
    if (schema_cache.count(table_name)) {
        DEBUG_CATALOG("Table '" << table_name << "' already exists");
        return false;
    }

    TableSchema schema{table_name, columns};
    Record record(schema.serialize());
    record_manager.insert_record(record);
    schema_cache[table_name] = schema;

    DEBUG_CATALOG("Table '" << table_name << "' created with columns: " << schema.serialize());
    return true;
}

bool CatalogManager::drop_table(const std::string& table_name) {
    DEBUG_CATALOG("Attempting to drop table '" << table_name << "'");

    if (!schema_cache.count(table_name)) {
        DEBUG_CATALOG("Table '" << table_name << "' does not exist");
        return false;
    }

    TableSchema schema = schema_cache[table_name];
    std::string serialized_schema = schema.serialize();

    RecordIterator iterator(record_manager.get_disk());
    bool found = false;

    while (iterator.has_next()) {
        auto [rec, page_id, slot_id] = iterator.next_with_location();
        if (rec.to_string() == serialized_schema) {
            RecordID rid(page_id, slot_id);
            int record_id = rid.encode();
            record_manager.delete_record(record_id);
            DEBUG_CATALOG("Deleted schema for '" << table_name << "' at page " << page_id << ", slot " << slot_id);
            found = true;
            break;
        }
    }

    if (!found) {
        DEBUG_CATALOG("Failed to find serialized schema for '" << table_name << "' to delete");
        return false;
    }

    TableManager tm(*this, record_manager, index_manager);  // Pass yourself as catalog manager
    int deleted = tm.delete_from(table_name, -1);  // Delete all records
    DEBUG_CATALOG("Deleted " << deleted << " data records from table '" << table_name << "'");

    schema_cache.erase(table_name);
    DEBUG_CATALOG("Table '" << table_name << "' dropped from cache (record data remains)");
    return true;
}

TableSchema CatalogManager::get_schema(const std::string& table_name) {
    DEBUG_CATALOG("Fetching schema for table '" << table_name << "'");
    if (!schema_cache.count(table_name)) {
        DEBUG_CATALOG("Table '" << table_name << "' not found in catalog");
        return TableSchema{}; // Return empty schema instead of throwing
    }
    return schema_cache[table_name];
}

std::vector<std::string> CatalogManager::list_tables() {
    std::vector<std::string> names;
    for (const auto& [name, _] : schema_cache) {
        names.push_back(name);
    }
    DEBUG_CATALOG("Listing tables: " << names.size() << " found");
    return names;
}

bool CatalogManager::column_exists(const std::string& table_name, const std::string& column_name) {
    if (!schema_cache.count(table_name)) return false;
    const auto& columns = schema_cache[table_name].columns;
    return std::find(columns.begin(), columns.end(), column_name) != columns.end();
}
