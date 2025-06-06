#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <set>

using namespace std;

class IndexManager {
private:
    // table -> column -> key -> set<record_id>
    unordered_map<string, unordered_map<string, map<string, set<int>>>> indexes;

    bool column_exists(const string& table_name, const string& column_name);

public:
    IndexManager() = default;

    bool create_index(const string& table_name, const string& column_name);
    bool drop_index(const string& table_name, const string& column_name);

    bool insert_entry(const string& table_name, const string& column_name, const string& key, int record_id);
    bool delete_entry(const string& table_name, const string& column_name, const string& key, int record_id);

    vector<int> search(const string& table_name, const string& column_name, const string& key);
    vector<int> range_search(const string& table_name, const string& column_name, const string& start_key, const string& end_key);
};
