#include "../header/Table.h"
#include <iostream>
#include <utility>

using namespace std;
using namespace cvc5;


Table::Table(Solver &solver, string table_name) : slv(solver) {
    tableName = std::move(table_name);
    DatatypeDecl tableDatatype = slv.mkDatatypeDecl(tableName);
    DatatypeConstructorDecl tableConstructor = slv.mkDatatypeConstructorDecl("make-" + tableName);

    auto columnInfo = TableInfo::getColumnInfo(tableName);
    columnNames = columnInfo;
    vector<Sort> tableSorts;
    for (const auto &entry: columnInfo) {
        tableConstructor.addSelector(entry.first, entry.second);
        tableSorts.push_back(entry.second);
    }
    tableDatatype.addConstructor(tableConstructor);
//    tableSort = slv.mkDatatypeSort(tableDatatype);


    //Change of Model Relation
//    tableSort = slv.mkTupleSort(tableSorts);

    // Change Model Bag
//    tableSort = slv.mkBagSort(slv.mkDatatypeSort(tableDatatype));
//    tableSort = slv.mkDatatypeSort(tableDatatype);

    // Change to Table
    Sort tupleSort = slv.mkTupleSort(tableSorts);
    tableSort = slv.mkBagSort(tupleSort);
}

Table::Table(Solver &solver, Table originalTable, const vector<std::string> &desiredColumns) : slv(solver) {
    tableName = std::move(originalTable.getTableName()) + "_projected";
//    DatatypeDecl tableDatatype = slv.mkDatatypeDecl(tableName);
    DatatypeConstructorDecl tableConstructor = slv.mkDatatypeConstructorDecl("make-" + tableName);

//    const Datatype& originalTableType = originalTable.getTableSort().getDatatype();
    // Bag
//    const Datatype& originalTableType = originalTable.getBagTableSort().getBagElementSort().getDatatype();
//
//    vector<Sort> tableSorts;
//    for (size_t i = 0; i < originalTableType.getNumConstructors(); ++i) {
//        const DatatypeConstructor& ctor = originalTableType[i];
//        for (size_t j = 0; j < ctor.getNumSelectors(); ++j) {
//            const DatatypeSelector& selector = ctor[j];
//            if (std::find(desiredColumns.begin(), desiredColumns.end(), selector.getName()) != desiredColumns.end()) {
//                // Add the selector to the new Table
////                tableConstructor.addSelector(selector.getName(), selector.getCodomainSort());
//                desiredColumnNames.emplace_back(selector.getName(), selector.getCodomainSort());
//                tableSorts.push_back(selector.getCodomainSort());
//            }
//        }
//    }
//    tableDatatype.addConstructor(tableConstructor);
//    tableSort = slv.mkDatatypeSort(tableDatatype);
    vector<pair<string, Sort>> desiredColumnNames;
    vector<Sort> tableSorts;
    for (const auto& columnName : originalTable.getColumnNames()) {
        // Check if the string value exists in desiredColumns
        if (find(desiredColumns.begin(), desiredColumns.end(), columnName.first) != desiredColumns.end()) {
            // If it exists, add to filteredColumnNames
            desiredColumnNames.push_back(columnName);
            tableSorts.push_back(columnName.second);
        }
    }
    columnNames = desiredColumnNames;
    Sort tupleSort = slv.mkTupleSort(tableSorts);
    tableSort = slv.mkBagSort(tupleSort);
}

Table::Table(Solver &solver, std::string tableName, const Sort& tableSort, vector<pair<string, Sort>> newColumnNames)  : slv(solver), tableSort(tableSort), tableName(std::move(tableName)) {
    columnNames = std::move(newColumnNames);
}

Term Table::createRow(const RowData &rowData) {
//    DatatypeConstructor tableCons = tableSort.getDatatype()[0];
    // Bag
    DatatypeConstructor tableCons = tableSort.getBagElementSort().getDatatype()[0];
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
    Term row = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, {termArguments});
//    Term setWithRow1 = slv.mkTerm(Kind::SET_SINGLETON, {row});
    // Bag
//    Term BagWithRow1 = slv.mkTerm(Kind::BAG_MAKE, {row, slv.mkInteger(1)});
    return row;
}
Sort Table::getBagTableSort() {
    // BAG
    return tableSort;
}

Sort Table::getTableSort() {
    // BAG
    return tableSort.getBagElementSort();
//    return tableSort;
}

std::string Table::getTableName() {
    return tableName;
}

vector<pair<string, Sort>> Table::getColumnNames() {
    return columnNames;
}

Term Table::createSet() {
    Sort addressTableSet = slv.mkSetSort(tableSort);
    Term addressTableTerm = slv.mkConst(addressTableSet, "addressTable");
    return addressTableTerm;
}

// Bag
Term Table::createBag() {
    Sort addressTableSet = slv.mkBagSort(tableSort);
    // Table
    Term addressTableTerm = slv.mkConst(tableSort, tableName);
//    Term addressTableTerm = slv.mkConst(addressTableSet, "addressTable");
//    Term addressTableTermBag = slv.mkTerm(Kind::BAG_MAKE, {addressTableTerm, slv.mkInteger(1)});
    return addressTableTerm;
}

Term Table::createEmptySet() {
    Sort addressTableSet = slv.mkSetSort(tableSort);
    Term emptyAddressTable = slv.mkEmptySet(addressTableSet);
    return emptyAddressTable;
}

// Table
int Table::getColumnIndexByName(const string& columnName) {
    for (int i = 0; i < columnNames.size(); ++i) {
        if (columnNames[i].first == columnName) {
            return i;
        }
    }
    throw std::runtime_error("Column name not found in table");
}

Term Table::accessFieldByIndex(const Term &row, const int &columnIndex) {
    // Get the datatype
//    cout << row.getSort().getBagElementSort().getTupleSorts()[0] << endl;
    Sort rowType = row.getSort().getTupleSorts()[columnIndex];
    const DatatypeConstructor& ctor = row.getSort().getDatatype()[0];

    if (columnIndex >= ctor.getNumSelectors()) {
        throw std::runtime_error("Index out of range for tuple");
    }

    const DatatypeSelector& selector = ctor[columnIndex];
    return slv.mkTerm(Kind::APPLY_SELECTOR, {selector.getTerm(), row});
}

Term Table::accessField(const Term &row, const string &fieldName) {
    // Get the datatype
    const Datatype& rowType = row.getSort().getDatatype();

    // Find the appropriate selector for the fieldName
    for (size_t i = 0; i < rowType.getNumConstructors(); ++i) {
        const DatatypeConstructor& ctor = rowType[i];
        for (size_t j = 0; j < ctor.getNumSelectors(); ++j) {
            const DatatypeSelector& selector = ctor[j];
            if (selector.getName() == fieldName) {
                // Apply the selector to the row
                return slv.mkTerm(Kind::APPLY_SELECTOR, {selector.getTerm(), row});
            }
        }
    }

    throw std::runtime_error("Field name not found in datatype");
}


