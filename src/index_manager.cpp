#include "../include/index_manager.h"
#include "../include/catalog_manager.h"
#include <iostream>
#include <algorithm>

using namespace std;

#define DEBUG_INDEX_MANAGER(msg) cout << "[DEBUG][INDEX_MANAGER] " << msg << endl;

IndexManager::IndexManager(CatalogManager& cm) : catalog_manager(cm) {}

bool IndexManager::column_exists(const string& table_name, const string& column_name) {
    try {
        TableSchema schema = catalog_manager.get_schema(table_name);
        bool exists = find(schema.columns.begin(), schema.columns.end(), column_name) != schema.columns.end();
        DEBUG_INDEX_MANAGER("Checking if column '" << column_name << "' exists in table '" << table_name << "': " << (exists ? "YES" : "NO"));
        return exists;
    } catch (const exception& e) {
        DEBUG_INDEX_MANAGER("Exception in column_exists: " << e.what());
        return false;
    } catch (...) {
        DEBUG_INDEX_MANAGER("Unknown exception in column_exists");
        return false;
    }
}

bool IndexManager::create_index(const string& table_name, const string& column_name) {
    DEBUG_INDEX_MANAGER("Creating index on table '" << table_name << "', column '" << column_name << "'");
    if (!column_exists(table_name, column_name)) {
        cout << "[IndexManager] Column does not exist: " << column_name << endl;
        DEBUG_INDEX_MANAGER("Failed to create index: column does not exist");
        return false;
    }
    indexes[table_name][column_name] = {};
    DEBUG_INDEX_MANAGER("Index created successfully");
    return true;
}

bool IndexManager::drop_index(const string& table_name, const string& column_name) {
    DEBUG_INDEX_MANAGER("Dropping index on table '" << table_name << "', column '" << column_name << "'");
    if (indexes.count(table_name) && indexes[table_name].count(column_name)) {
        indexes[table_name].erase(column_name);
        if (indexes[table_name].empty()) {
            indexes.erase(table_name);
        }
        DEBUG_INDEX_MANAGER("Index dropped successfully");
        return true;
    }
    DEBUG_INDEX_MANAGER("Index not found to drop");
    return false;
}

bool IndexManager::insert_entry(const string& table_name, const string& column_name, const string& key, int record_id) {
    DEBUG_INDEX_MANAGER("Inserting entry: table='" << table_name << "', column='" << column_name << "', key='" << key << "', record_id=" << record_id);
    if (!column_exists(table_name, column_name)) {
        DEBUG_INDEX_MANAGER("Insert failed: column does not exist");
        return false;
    }
    indexes[table_name][column_name][key].insert(record_id);
    DEBUG_INDEX_MANAGER("Entry inserted successfully");
    return true;
}

bool IndexManager::delete_entry(const string& table_name, const string& column_name, const string& key, int record_id) {
    DEBUG_INDEX_MANAGER("Deleting entry: table='" << table_name << "', column='" << column_name << "', key='" << key << "', record_id=" << record_id);
    if (!column_exists(table_name, column_name)) {
        DEBUG_INDEX_MANAGER("Delete failed: column does not exist");
        return false;
    }
    auto& id_set = indexes[table_name][column_name][key];
    id_set.erase(record_id);
    if (id_set.empty()) {
        indexes[table_name][column_name].erase(key);
        DEBUG_INDEX_MANAGER("Key '" << key << "' erased from index as it became empty");
    }
    DEBUG_INDEX_MANAGER("Entry deleted successfully");
    return true;
}

vector<int> IndexManager::search(const string& table_name, const string& column_name, const string& key) {
    DEBUG_INDEX_MANAGER("Searching for key '" << key << "' in table '" << table_name << "', column '" << column_name << "'");
    vector<int> result;
    if (!column_exists(table_name, column_name)) {
        DEBUG_INDEX_MANAGER("Search failed: column does not exist");
        return result;
    }
    auto& id_set = indexes[table_name][column_name][key];
    result.assign(id_set.begin(), id_set.end());
    DEBUG_INDEX_MANAGER("Search found " << result.size() << " record(s)");
    return result;
}

vector<int> IndexManager::range_search(const string& table_name, const string& column_name, const string& start_key, const string& end_key) {
    DEBUG_INDEX_MANAGER("Range search: table='" << table_name << "', column='" << column_name << "', start_key='" << start_key << "', end_key='" << end_key << "'");
    vector<int> result;
    if (!column_exists(table_name, column_name)) {
        DEBUG_INDEX_MANAGER("Range search failed: column does not exist");
        return result;
    }
    auto& col_index = indexes[table_name][column_name];
    for (auto it = col_index.lower_bound(start_key); it != col_index.upper_bound(end_key); ++it) {
        result.insert(result.end(), it->second.begin(), it->second.end());
    }
    DEBUG_INDEX_MANAGER("Range search found " << result.size() << " record(s)");
    return result;
}
