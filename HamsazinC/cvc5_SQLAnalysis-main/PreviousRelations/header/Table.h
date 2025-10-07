#ifndef TPC_CPP_TABLE_H
#define TPC_CPP_TABLE_H

#include <cvc5/cvc5.h>
#include <vector>
#include <string>
#include "Types.h"

using namespace cvc5;

class Table {
private:
    Solver& slv;
    std::vector<Term> rows;
    Sort tableSort;
    std::string tableName;

public:
    Table(Solver& solver, std::string table_name);
    Table(Solver& solver, std::string tableName, const Sort& tableSort);
    void addRow(const RowData& rowData);

    void printRows() const;
    void printRowsForColumn(const std::string& columnName);
    void printColumnNames();

    std::string getTableName();
    std::vector<Term> getRows();
    Solver& getSolver();
    Sort getTableSort();
    int getNumColumns();
    std::vector<std::string> getColumnNames();
    int getColumnIndex(const std::string& columnName);
    Sort getColumnSort(const std::string& columnName);

//    int getColumnIndex(const std::string& columnName);
    std::vector<int> getColumnIndices(const std::vector<std::string>& columnNames);

    void resetDatatype(const Sort& newSort);
    void resetRows(const std::vector<Term>& newRows);

};


#endif //TPC_CPP_TABLE_H
