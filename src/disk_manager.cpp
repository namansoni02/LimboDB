#include "../include/disk_manager.h"
#include<iostream>
#include <sys/stat.h> // or use platform-specific API

using namespace std;

DiskManager::DiskManager(const string& filename) : file_name(filename) {
    db_file.open(filename, ios::in | ios::out | ios::binary);
    if(!db_file.is_open()){
        db_file.open(filename, std::ios::out | std::ios::binary); // Create the file if it doesn't exist
        std::vector<char> zero_page(PAGE_SIZE, 0);
        db_file.write(zero_page.data(), PAGE_SIZE);
        db_file.close();
        db_file.open(filename, ios::in | ios::out | ios::binary);
    }

}

DiskManager::~DiskManager() {
    flush();
    db_file.close();
}

bool DiskManager::write_page(int page_id, const vector<char>& data) {
    cout << "[DEBUG] Writing page " << page_id << "\n";
    db_file.clear(); // reset stream state before seekp

    db_file.seekp(page_id * PAGE_SIZE, ios::beg);
    if (!db_file) {
        cerr << "[ERROR] Seekp failed for page " << page_id << "\n";
        return false;
    }

    db_file.write(data.data(), PAGE_SIZE);
    if (!db_file) {
        cerr << "[ERROR] Write failed for page " << page_id << "\n";
        return false;
    }

    db_file.flush();
    if (!db_file) {
        cerr << "[ERROR] Flush failed for page " << page_id << "\n";
        return false;
    }

    cout << "[DEBUG] Page " << page_id << " written successfully.\n";
    return true;
}

vector<char> DiskManager::read_page(int page_id){
    vector<char> buffer (PAGE_SIZE);
    db_file.clear(); // reset before seekg
    db_file.seekg(page_id * PAGE_SIZE, ios::beg);
    if (!db_file) {
        cerr << "[ERROR] Seekg failed for page " << page_id << "\n";
        return buffer;
    }
    db_file.read(buffer.data(), PAGE_SIZE);
    return buffer;
}


void DiskManager::flush(){
    db_file.flush();
}