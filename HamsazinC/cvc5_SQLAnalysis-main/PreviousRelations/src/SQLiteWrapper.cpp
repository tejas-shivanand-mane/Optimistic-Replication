#include "../header/SQLiteWrapper.h"
#include <iostream>
#include <utility>
#include "../header/Types.h"

SQLiteWrapper::SQLiteWrapper(std::string  path) : db(nullptr), db_path(std::move(path)) {}

SQLiteWrapper::~SQLiteWrapper() {
    if (db) {
        sqlite3_close(db);
    }
}

bool SQLiteWrapper::connect() {
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }
    return true;
}

std::vector<RowData> SQLiteWrapper::executeQuery(const std::string& sql) {
    std::vector<RowData> results;
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to fetch data: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return results;
    }

    int col_count = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RowData rowData;
        for (int i = 0; i < col_count; ++i) {
            int col_type = sqlite3_column_type(stmt, i);
            if (col_type == SQLITE_INTEGER) {
                rowData.emplace_back(sqlite3_column_int64(stmt, i));
            } else if (col_type == SQLITE_TEXT) {
                rowData.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i)));
            } else if (col_type == SQLITE_FLOAT) {
                rowData.emplace_back(sqlite3_column_double(stmt, i));
            }
            // Add further type handling if needed, like for SQLITE_FLOAT, etc.
        }
        results.push_back(rowData);
    }

    sqlite3_finalize(stmt);
    return results;
}

