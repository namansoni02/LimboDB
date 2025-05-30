#pragma once
#include <unordered_map>
#include "./record_manager.h"
#include <string>
#include <vector>

using namespace std;

struct TableSchema {
    string table_name;
    vector<string> columns;

    string serialize() const;
    static TableSchema deserialize(const string& record_str);
};

class CatalogManager { 
private:
    RecordManager record_manager;
    unordered_map<string, TableSchema> schema_cache;

    void load_catalog();
public:
    CatalogManager(RecordManager& rm);


    bool create_table(const string& table_name, const vector<string>& columns);
    bool drop_table(const string& table_name);
    TableSchema get_schema(const string& table_name);
    vector<string> list_tables();
};