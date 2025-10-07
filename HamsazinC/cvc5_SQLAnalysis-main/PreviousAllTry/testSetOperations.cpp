#include <iostream>

#include <cvc5/cvc5.h>
#include "header/Table.h"
#include "header/TableInfo.h"
#include "header/RelationalAlgebras.h"
#include "header/RelationalProver.h"

using namespace std;
using namespace cvc5;

int main() {
    Solver slv;
    slv.setLogic("HO_ALL"); // Set the logic
    slv.setOption("produce-models", "true");
    slv.setOption("output-language", "smt2");
    slv.setOption("sets-ext", "true");

    slv.setOption("tlimit", "10000"); // 10 seconds

//    Table customerTable(slv, "customer");
    Table addressTable(slv, "address");
//    Table brokerTable(slv, "broker");
    Table zipCodeTable(slv, "zip_code");

//    Term addressTableTerm = addressTable.createSet();
//    Term zipCodeTableTerm = zipCodeTable.createSet();

    // Bag
    Term addressTableTerm = addressTable.createBag();
    Term zipCodeTableTerm = zipCodeTable.createBag();

    // Test Inserts
//    RowData addressRow1 = {1, "Row1Name", "Row1Name", "Row1Name", "Row1Name"};
//    RowData addressRow2 = {2, "Row2Name", "Row2Name", "Row2Name", "Row2Name"};
//    //Assert Insert Over Insert State Commutivity
//    Term stateCommutivityForInserts = RelationalProver::stateOfInsertOverInsert(slv, addressTable, addressTableTerm, addressRow1, addressRow2);
//    slv.assertFormula(stateCommutivityForInserts);

    auto selectTerm = RelationalAlgebras::Select(slv, addressTable, addressTableTerm, "ad_ctry", "USA", "eq");
    cout << "SELECT1 RETURNED: " << selectTerm << endl;
    auto selectTerm2 = RelationalAlgebras::Select(slv, addressTable, selectTerm, "ad_zc_code", "M4J 1M9", "eq");
    cout << "SELECT1 RETURNED: " << selectTerm2 << endl;

    vector<string> projectColumns = {"ad_ctry", "ad_zc_code"};
    ProjectionReturn projectReturn = RelationalAlgebras::Project(slv, addressTable, selectTerm2, projectColumns);
    Table projectionNewTable = projectReturn.newTable;
    Term projectionTerm = projectReturn.setComprehension;
    cout << "Projection RETURNED: " << projectionTerm << " / " << projectionTerm.getSort() << endl;

    // Now solve and check if the solver finds that finalTable1 is equal to finalTable2

    RowData addressRow1 = {"USA", "M4J 1M9"};
    RowData addressRow2 = {1, "Row1Name", "Row1Name", "M4J 1M9", "USA"};
    Term newRow = addressTable.createRow(addressRow2);
    Term newRow2 = projectionNewTable.createRow(addressRow1);


//    Term isMember = slv.mkTerm(Kind::SET_MEMBER, {newRow, selectTerm2});
    // Bag
    Term isMember = slv.mkTerm(Kind::BAG_MEMBER, {newRow, selectTerm2});
    slv.assertFormula(isMember);

    ProjectionReturn joined = RelationalAlgebras::Product(slv, projectionNewTable, zipCodeTable, projectionTerm, zipCodeTableTerm);
    Term productTerm = joined.setComprehension;
    Table productTable = joined.newTable;
    cout << "Table Product RETURNED: " << joined.setComprehension << " / " << productTerm.getSort() << endl;

    RowData addressRow3 = {1, "Row1Name", "Row1Name", "M4J 1M9", "USA", "Row1Name", "Row1Name", "Row1Name"};
    RowData addressRow4 = {"M4J 1M9", "USA", "Row1Name", "Row1Name", "Row1Name"};
//    Term addressRow3Set = productTable.createRow(addressRow3);
    Term addressRow4Set = productTable.createRow(addressRow4);

//    Term isMember = slv.mkTerm(Kind::SET_MEMBER, {addressRow3Set, joined.setComprehension});
    Term isMember2 = slv.mkTerm(Kind::BAG_MEMBER, {addressRow4Set, productTerm});
    slv.assertFormula(isMember2);
    Result result = slv.checkSat();
    cout << "cvc5 reports: " << " " << " is "
         << result << "." << endl;
    if (result.isSat()) {
        cout << "For instance, " <<  slv.getValue(addressTableTerm) << " is a member." << endl;
    }

    return 0;
}
