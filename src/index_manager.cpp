#include "../include/index_manager.h"
#include <iostream>
#include <algorithm>

using namespace std;

#define DEBUG_INDEX_MANAGER(msg) cout << "[DEBUG][INDEX_MANAGER] " << msg << endl;


// Create index
bool IndexManager::create_index(const string& table_name, const string& column_name) {
    DEBUG_INDEX_MANAGER("Creating index on table '" << table_name << "', column '" << column_name << "'");
    indexes[table_name][column_name] = {}; // initialize empty index
    DEBUG_INDEX_MANAGER("Index created successfully");
    return true;
}

// Drop index
bool IndexManager::drop_index(const string& table_name, const string& column_name) {
    DEBUG_INDEX_MANAGER("Dropping index on table '" << table_name << "', column '" << column_name << "'");
    
    // Declare iterator first
    auto table_it = indexes.find(table_name);
    if (table_it != indexes.end()) {
        auto col_it = table_it->second.find(column_name);  // Explicit declaration
        if (col_it != table_it->second.end()) {
            table_it->second.erase(col_it);
            if (table_it->second.empty()) {
                indexes.erase(table_it);
            }
            DEBUG_INDEX_MANAGER("Index dropped successfully");
            return true;
        }
    }
    DEBUG_INDEX_MANAGER("Index not found to drop");
    return false;
}


// Insert entry
bool IndexManager::insert_entry(const string& table_name, const string& column_name, const string& key, int record_id) {
    DEBUG_INDEX_MANAGER("Inserting entry: table='" << table_name << "', column='" << column_name << "', key='" << key << "', record_id=" << record_id);
    indexes[table_name][column_name][key].insert(record_id);
    DEBUG_INDEX_MANAGER("Entry inserted successfully");
    return true;
}

// Delete entry
bool IndexManager::delete_entry(const string& table_name, const string& column_name, const string& key, int record_id) {
    DEBUG_INDEX_MANAGER("Deleting entry: table='" << table_name << "', column='" << column_name << "', key='" << key << "', record_id=" << record_id);
    auto& id_set = indexes[table_name][column_name][key];
    id_set.erase(record_id);
    if (id_set.empty()) {
        indexes[table_name][column_name].erase(key);
        DEBUG_INDEX_MANAGER("Key '" << key << "' erased from index as it became empty");
    }
    DEBUG_INDEX_MANAGER("Entry deleted successfully");
    return true;
}

// Search by key
vector<int> IndexManager::search(const string& table_name, const string& column_name, const string& key) {
    DEBUG_INDEX_MANAGER("Searching for key '" << key << "' in table '" << table_name << "', column '" << column_name << "'");
    vector<int> result;
    auto& id_set = indexes[table_name][column_name][key];
    result.assign(id_set.begin(), id_set.end());
    DEBUG_INDEX_MANAGER("Search found " << result.size() << " record(s)");
    return result;
}

// Range search
vector<int> IndexManager::range_search(const string& table_name, const string& column_name, const string& start_key, const string& end_key) {
    DEBUG_INDEX_MANAGER("Range search: table='" << table_name << "', column='" << column_name << "', start_key='" << start_key << "', end_key='" << end_key << "'");
    vector<int> result;
    auto& col_index = indexes[table_name][column_name];
    for (auto it = col_index.lower_bound(start_key); it != col_index.upper_bound(end_key); ++it) {
        result.insert(result.end(), it->second.begin(), it->second.end());
    }
    DEBUG_INDEX_MANAGER("Range search found " << result.size() << " record(s)");
    return result;
}
