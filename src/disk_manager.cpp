#define COLOR_DEBUG   "\033[1;36m" // Cyan
#define COLOR_ERROR   "\033[1;31m" // Red
#define COLOR_SUCCESS "\033[1;32m" // Green
#define COLOR_RESET   "\033[0m"

#include "../include/disk_manager.h"
#include <iostream>
#include <sys/stat.h>

using namespace std;

DiskManager::DiskManager(const string& filename) : file_name(filename) {
    cout << COLOR_DEBUG << "[DEBUG][DISK_MANAGER] DiskManager constructor called with file: " << filename << COLOR_RESET << endl;
    db_file.open(filename, ios::in | ios::out | ios::binary);
    if(!db_file.is_open()){
        cout << COLOR_DEBUG << "[DEBUG][DISK_MANAGER] File does not exist. Creating new file: " << filename << COLOR_RESET << endl;
        db_file.open(filename, std::ios::out | std::ios::binary);
        std::vector<char> zero_page(PAGE_SIZE, 0);
        db_file.write(zero_page.data(), PAGE_SIZE);
        db_file.close();
        db_file.open(filename, ios::in | ios::out | ios::binary);
    }
}

DiskManager::~DiskManager() {
    cout << COLOR_DEBUG << "[DEBUG][DISK_MANAGER] DiskManager destructor called." << COLOR_RESET << endl;
    flush();
    db_file.close();
}

bool DiskManager::write_page(int page_id, const vector<char>& data) {
    cout << COLOR_DEBUG << "[DEBUG][DISK_MANAGER] Writing page " << page_id << COLOR_RESET << endl;
    db_file.clear();

    db_file.seekp(page_id * PAGE_SIZE, ios::beg);
    if (!db_file) {
        cerr << COLOR_ERROR << "[DEBUG][DISK_MANAGER] [ERROR] Seekp failed for page " << page_id << COLOR_RESET << "\n";
        return false;
    }

    db_file.write(data.data(), PAGE_SIZE);
    if (!db_file) {
        cerr << COLOR_ERROR << "[DEBUG][DISK_MANAGER] [ERROR] Write failed for page " << page_id << COLOR_RESET << "\n";
        return false;
    }

    db_file.flush();
    if (!db_file) {
        cerr << COLOR_ERROR << "[DEBUG][DISK_MANAGER] [ERROR] Flush failed for page " << page_id << COLOR_RESET << "\n";
        return false;
    }

    cout << COLOR_SUCCESS << "[DEBUG][DISK_MANAGER] Page " << page_id << " written successfully." << COLOR_RESET << endl;
    return true;
}

std::vector<char> DiskManager::read_page(int page_id) {
    cout << COLOR_DEBUG << "[DEBUG][DISK_MANAGER] Reading page " << page_id << COLOR_RESET << endl;
    std::vector<char> page(PAGE_SIZE);

    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(string(COLOR_ERROR) + "[DEBUG][DISK_MANAGER] Failed to open file" + COLOR_RESET);
    }

    file.seekg(page_id * PAGE_SIZE);
    if (!file.good()) {
        std::cerr << COLOR_ERROR << "[DEBUG][DISK_MANAGER] [ERROR] Seekg failed for page " << page_id << COLOR_RESET << std::endl;
        throw std::runtime_error(string(COLOR_ERROR) + "[DEBUG][DISK_MANAGER] Seekg failed" + COLOR_RESET);
    }

    file.read(page.data(), PAGE_SIZE);
    if (file.gcount() < PAGE_SIZE) {
        std::cerr << COLOR_ERROR << "[DEBUG][DISK_MANAGER] [ERROR] Could not read full page " << page_id << COLOR_RESET << std::endl;
        throw std::runtime_error(string(COLOR_ERROR) + "[DEBUG][DISK_MANAGER] Partial read" + COLOR_RESET);
    }

    cout << COLOR_SUCCESS << "[DEBUG][DISK_MANAGER] Page " << page_id << " read successfully." << COLOR_RESET << endl;
    return page;
}

void DiskManager::flush(){
    cout << COLOR_DEBUG << "[DEBUG][DISK_MANAGER] Flushing db_file." << COLOR_RESET << endl;
    db_file.flush();
}

int DiskManager::get_num_pages() {
    cout << COLOR_DEBUG << "[DEBUG][DISK_MANAGER] Getting number of pages." << COLOR_RESET << endl;
    db_file.clear();
    db_file.seekg(0, ios::end);
    streampos file_size = db_file.tellg();

    if (file_size == -1) {
        cerr << COLOR_ERROR << "[DEBUG][DISK_MANAGER] [ERROR] Failed to get file size." << COLOR_RESET << "\n";
        return -1;
    }
    int num_pages = static_cast<int>(file_size / PAGE_SIZE);
    cout << COLOR_SUCCESS << "[DEBUG][DISK_MANAGER] Number of pages: " << num_pages << COLOR_RESET << endl;
    return num_pages;
}

int DiskManager::allocate_page() {
    cout << COLOR_DEBUG << "[DEBUG][DISK_MANAGER] Allocating new page." << COLOR_RESET << endl;
    int new_page_id = get_num_pages();

    vector<char> zero_page(PAGE_SIZE, 0);
    if (!write_page(new_page_id, zero_page)) {
        cerr << COLOR_ERROR << "[DEBUG][DISK_MANAGER] [ERROR] Failed to write zero page for allocation." << COLOR_RESET << "\n";
        return -1;
    }

    cout << COLOR_SUCCESS << "[DEBUG][DISK_MANAGER] Allocated new page with ID " << new_page_id << "." << COLOR_RESET << endl;
    return new_page_id;
}
