#ifndef TPC_CPP_TABLE_H
#define TPC_CPP_TABLE_H

#include <cvc5/cvc5.h>
#include <vector>
#include <string>
#include <algorithm>
#include "Types.h"
#include "TableInfo.h"

using namespace cvc5;

class Table {
private:
    Solver* slv;
    Sort tableSort;
    std::string tableName;
    vector<pair<string, Sort>> columnNames;

public:
    Table(Solver &solver, std::string table_name);
    Table(Solver &solver, Table originalTable, const std::vector<std::string> &desiredColumns);
    Table(Solver &solver, std::string tableName, const Sort& tableSort, vector<pair<string, Sort>> newColumnNames);

    Table (const Table& other);
    Table& operator=(const Table& other);
    Table() : slv(new Solver()) {};

    explicit Table(Solver& slv) : slv(&slv) {};

    Term createRow(const RowData& rowData);
    Term createBag();
    Term createEmptyBag();
    Term createDefaultBag();
    Sort getTableSort();
    Sort getBagTableSort();
    vector<uint32_t> getIndicesOfColumns(const vector<std::string>& desiredColumnNames);
    std::string getTableName();
    vector<string> getTableColumnNames();
    vector<pair<string, Sort>> getColumnNames();
    int getColumnIndexByName(const string& columnName);

    Term accessField(const Term& row, const std::string& fieldName);
    Term accessFieldByIndex(const Term& row, const int& columnIndex);
};


#endif //TPC_CPP_TABLE_H
