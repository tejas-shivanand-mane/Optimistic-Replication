#include <iostream>
#include <vector>
#include <string>
#include <sqlite3.h>
#include <cvc5/cvc5.h>

#include "header/SQLiteWrapper.h"
#include "header/Table.h"
#include "header/Types.h"
#include "header/RelationalOperations.h"

using namespace std;
using namespace cvc5;

void initSingleTable(SQLiteWrapper &db, Table *table) {
    std::string tableName = table->getTableName();
//    std::string sql = "SELECT * FROM " + tableName + " LIMIT 100";
    std::string sql = "SELECT * FROM " + tableName;
    auto results = db.executeQuery(sql);
    for (const auto &rowData: results) {
        table->addRow(rowData);
    }
}

void initTables(SQLiteWrapper &db, vector<Table *> &tables) {
    for (auto &table: tables) {
        initSingleTable(db, table);
    }
}


int test7() {
    Solver slv;

    slv.setOption("produce-models", "true");

//    RelationalOperations operations(slv);

    SQLiteWrapper db("../st.db");
    if (!db.connect()) {
        return 1;
    }

    // Create Tables
    Table customerTable(slv, "customer");
    Table addressTable(slv, "address");
    Table brokerTable(slv, "broker");
    Table zipCodeTable(slv, "zip_code");
//    Table companyTable(slv, "company");
    vector<Table *> tables = {&customerTable, &addressTable, &brokerTable, &zipCodeTable};

    // Init Tables
    initTables(db, tables);

    // Test Tables
//    addressTable.printRows();
//    addressTable.printRowsForColumn("ad_line1");

//    RelationalOperations::selection(addressTable, "ad_ctry", "eq", slv.mkString("USA"));

//    vector<string> projectedColumns = {"ad_ctry", "ad_line1", "ad_zc_code"};
//    RelationalOperations::projection(addressTable, projectedColumns);

//    RelationalOperations::rename(addressTable, "ad_ctry", "country");
//    RelationalOperations::selection(addressTable, "country", "eq", slv.mkString("USA"));

//    Table joinedTable = RelationalOperations::innerJoin(addressTable, zipCodeTable, "ad_zc_code", "zc_code");

//    Table unionOfAddresses = RelationalOperations::unionTwo(addressTable, addressTable);

    Table groupedTable = RelationalOperations::groupByAggregate(addressTable, {"ad_id", "min"}, "ad_ctry");

    cout << "Number of Rows: " << groupedTable.getRows().size() << endl;
    for (const auto &row: groupedTable.getRows()) {
        cout << row << endl;
    }
    groupedTable.printColumnNames();
    return 0;
}
