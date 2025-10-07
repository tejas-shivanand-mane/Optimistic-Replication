#ifndef SQLITE_WRAPPER_H
#define SQLITE_WRAPPER_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include "Types.h"

class SQLiteWrapper {
private:
    sqlite3* db;
    std::string db_path;

public:

    SQLiteWrapper(std::string  db_path);
    ~SQLiteWrapper();

    bool connect();
    std::vector<RowData> executeQuery(const std::string& sql);
};

#endif // SQLITE_WRAPPER_H
