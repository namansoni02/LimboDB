#include "./include/query/query_parser.h"
#include "./include/disk_manager.h"
#include "./include/record_manager.h"
#include "./include/catalog_manager.h"
#include "./include/table_manager.h"
#include "./include/index_manager.h"

int main() {
    DiskManager disk_manager("database.db");
    RecordManager record_manager(disk_manager);

    CatalogManager catalog_manager( record_manager);
    IndexManager index_manager(catalog_manager);
    TableManager table_manager(catalog_manager, record_manager, index_manager);

    QueryParser parser(catalog_manager, table_manager, index_manager);
    parser.run_interactive();

    return 0;
}
