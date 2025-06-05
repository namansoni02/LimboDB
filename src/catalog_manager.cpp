#include "../include/catalog_manager.h"
#include <iostream>
#include <sstream>
#include "../include/record_iterator.h"
#include "../include/table_manager.h"
#include <algorithm>

#define DEBUG_CATALOG(msg) std::cout << "[DEBUG][CATALOG_MANAGER] " << msg << std::endl;

// ---------- TableSchema Methods ----------

std::string TableSchema::serialize() const {
    std::ostringstream oss;
    oss << table_name << "|";
    for (size_t i = 0; i < columns.size(); ++i) {
        oss << columns[i];
        if (i < columns.size() - 1) oss << ",";
    }
    DEBUG_CATALOG("Serialized schema for table '" << table_name << "': " << oss.str());
    return oss.str();
}

TableSchema TableSchema::deserialize(const std::string& record_str) {
    size_t sep = record_str.find('|');
    if (sep == std::string::npos) {
        DEBUG_CATALOG("Failed to deserialize: missing separator in '" << record_str << "'");
        throw std::runtime_error("Invalid schema record format");
    }
    TableSchema schema;
    schema.table_name = record_str.substr(0, sep);
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
        Record rec = iter.next();
        try {
            TableSchema schema = TableSchema::deserialize(rec.to_string());
            schema_cache[schema.table_name] = schema;
            ++count;
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
    while (iterator.has_next()) {
        Record rec = iterator.next();
        std::string rec_str = rec.to_string();
        if (rec_str == serialized_schema) {
            RecordID rid(page_id, slot_id);
            record_manager.delete_record(rid);
            DEBUG_CATALOG("Deleted schema for '" << table_name << "' at " << rid);
            break;
        }
    }

    TableManager table_manager(*this, record_manager, index_manager);
    int deleted = 0;
    deleted = table_manager.delete_from(table_name, [](const std::vector<std::string>&) -> bool {
        return true;
    });
    DEBUG_CATALOG("Deleted " << deleted << " records from table '" << table_name << "'");

    schema_cache.erase(table_name);
    DEBUG_CATALOG("Table '" << table_name << "' dropped from cache and all records deleted");

    return true;
}

TableSchema CatalogManager::get_schema(const std::string& table_name) {
    DEBUG_CATALOG("Fetching schema for table '" << table_name << "'");
    if (!schema_cache.count(table_name)) {
        DEBUG_CATALOG("Table '" << table_name << "' not found in catalog");
        throw std::runtime_error("Table not found in catalog");
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

// ---------- Column Validation Helper ----------

bool CatalogManager::column_exists(const std::string& table_name, const std::string& column_name) {
    if (!schema_cache.count(table_name)) return false;
    const auto& columns = schema_cache[table_name].columns;
    return std::find(columns.begin(), columns.end(), column_name) != columns.end();
}
