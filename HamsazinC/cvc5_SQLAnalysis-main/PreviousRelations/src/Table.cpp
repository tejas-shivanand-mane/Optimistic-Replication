#include "../header/Table.h"
#include <iostream>
#include <utility>
#include "../header/TableInfo.h"
#include "../header/Types.h"

using namespace std;
using namespace cvc5;


Table::Table(Solver &solver, string table_name) : slv(solver) {
    tableName = std::move(table_name);
    DatatypeDecl tableDatatype = slv.mkDatatypeDecl(tableName);
    DatatypeConstructorDecl tableConstructor = slv.mkDatatypeConstructorDecl("make-" + tableName);

    auto columnInfo = TableInfo::getColumnInfo(tableName);
    for (const auto &entry: columnInfo) {
        tableConstructor.addSelector(entry.first, entry.second);
    }
    tableDatatype.addConstructor(tableConstructor);
    tableSort = slv.mkDatatypeSort(tableDatatype);
}


Table::Table(Solver &solver, std::string tableName, const Sort& tableSort)  : slv(solver), tableSort(tableSort), tableName(std::move(tableName)) {}

void Table::addRow(const RowData &rowData) {
    DatatypeConstructor tableCons = tableSort.getDatatype()[0];
    Term makeTableTerm = tableCons.getTerm();

    std::vector<Term> termArguments;
    termArguments.push_back(makeTableTerm);

    for (const auto &data: rowData) {
        if (std::holds_alternative<int64_t>(data)) {
//            cout << std::get<int64_t>(data) << endl;
            termArguments.push_back(slv.mkInteger(std::get<int64_t>(data)));
        } else if (std::holds_alternative<std::string>(data)) {
//            cout << std::get<std::string>(data) << endl;
            termArguments.push_back(slv.mkString(std::get<std::string>(data)));
        } else if (std::holds_alternative<double>(data)) {
//            cout << std::get<double>(data) << endl;
            double value = std::get<double>(data);
            int64_t denominator = 100; // e.g., if you want two decimal places
            auto numerator = static_cast<int64_t>(value * denominator);
            termArguments.push_back(slv.mkReal(numerator, denominator));
        }
        // Extend this for other types as necessary
    }

    Term row = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, termArguments);

    rows.push_back(row);
}


void Table::printRows() const {
    for (const auto &row: rows) {
        cout << row << endl;
    }
}


void Table::printColumnNames() {
    const Datatype& datatype = tableSort.getDatatype();
    const DatatypeConstructor& constructor = datatype[0];

    std::cout << "Number of selectors (columns) in datatype: " << constructor.getNumSelectors() << std::endl;

    for (size_t i = 0; i < constructor.getNumSelectors(); ++i) {
        std::cout << "Column " << i << ": " << constructor[i].getName() << std::endl;
    }
}


int Table::getColumnIndex(const std::string& columnName) {
    const Datatype& datatype = tableSort.getDatatype();
    const DatatypeConstructor& constructor = datatype[0];

    for (size_t i = 0; i < constructor.getNumSelectors(); ++i) {
        if(columnName == constructor[i].getName()){
            return i;
        }
    }
    return -1;
}


Sort Table::getColumnSort(const string &columnName) {
    const Datatype& datatype = tableSort.getDatatype();
    const DatatypeConstructor& constructor = datatype[0];

    for (size_t i = 0; i < constructor.getNumSelectors(); ++i) {
        if(columnName == constructor[i].getName()){
            return constructor[i].getCodomainSort();
        }
    }
    return Sort();
}

std::vector<std::string> Table::getColumnNames() {
    const Datatype& datatype = tableSort.getDatatype();
    const DatatypeConstructor& constructor = datatype[0];

    std::vector<std::string> columnNames;
    for (size_t i = 0; i < constructor.getNumSelectors(); ++i) {
        columnNames.push_back(constructor[i].getName());
    }
    return columnNames;
}

int Table::getNumColumns() {
    const Datatype& datatype = tableSort.getDatatype();
    const DatatypeConstructor& constructor = datatype[0];

    return constructor.getNumSelectors();
}

string Table::getTableName() {
    return tableName;
}

Solver& Table::getSolver() {
    return slv;
}

std::vector<Term> Table::getRows() {
    return rows;
}

Sort Table::getTableSort() {
    return tableSort;
}

std::vector<int> Table::getColumnIndices(const std::vector<std::string>& columnNames) {
    const Datatype& datatype = tableSort.getDatatype();
    const DatatypeConstructor& constructor = datatype[0];

    // Use an unordered_map for faster lookup of column names to indices
    std::unordered_map<std::string, int> columnNameToIndexMap;
    for (size_t i = 0; i < constructor.getNumSelectors(); ++i) {
        columnNameToIndexMap[constructor[i].getName()] = i;
    }

    std::vector<int> columnIndices;
    columnIndices.reserve(columnNames.size());

    // Now look up each requested column name and add its index to the vector
    for (const std::string& columnName : columnNames) {
        auto it = columnNameToIndexMap.find(columnName);
        if (it != columnNameToIndexMap.end()) {
            // The column was found, add the index to the vector
            columnIndices.push_back(it->second);
        } else {
            // The column wasn't found, handle the error as needed
            std::cerr << "Column " << columnName << " not found." << std::endl;
            columnIndices.push_back(-1);  // Using -1 as a sentinel for "not found"
        }
    }

    return columnIndices;
}

//int Table::getColumnIndex(const std::string &columnName) {
//    // First, find the index of the column by looking at the datatype declaration
//    const Datatype &datatype = tableSort.getDatatype();
//    const DatatypeConstructor &constructor = datatype[0];
//    cout << constructor.getNumSelectors() << endl;
//    cout << constructor[1].getName() << endl;
//    int columnIndex = -1;
//    for (size_t i = 0; i < constructor.getNumSelectors(); ++i) {
//        if (constructor[i].getName() == columnName) {
//            // We found the column; its index is i.
//            columnIndex = i;
//            return columnIndex;
//        }
//    }
//
//    cout << "Column " << columnName << " not found." << endl;
//    return columnIndex;
//}


void Table::printRowsForColumn(const std::string &columnName) {

//    int columnIndex = getColumnIndex(columnName);
    std::vector<std::string> columnNamesVector(1, columnName);
    int columnIndex = getColumnIndices(columnNamesVector)[0];
    // Now, print the value for this column in each row
    for (const auto &row: rows) {
        // The children of the row term start after the constructor itself,
        // hence the columnIndex + 1.
        Term columnValueTerm = row[columnIndex + 1];
        cout << columnValueTerm << endl;
    }
}

void Table::resetDatatype(const Sort& newSort) {
    this->tableSort = newSort;
}

void Table::resetRows(const std::vector<Term>& newRows) {
    this->rows = newRows;
}




