#pragma once
#include<string>
#include<fstream>
#include<vector>

using namespace std;

const int PAGE_SIZE = 4096;

class DiskManager{
private:
    fstream db_file;
    string file_name;

public:
    DiskManager(const std::string& filename);
    ~DiskManager();

    bool write_page(int page_id, const vector<char>& data);
    vector<char> read_page(int page_id);
    void flush();
};