#include "../header/RelationalProver.h"

Term RelationalProver::stateOfInsertOverInsert(Solver &slv, Table tableClass, Term tableTerm, RowData row11, RowData row2) {
    //Create Rows
    RowData addressDefaultRow = {0, "", "", "", ""};
    Term defaultRowSet = tableClass.createRow(addressDefaultRow);
    Term EmptyTable = tableClass.createEmptySet();

    Term addressRow1Set = tableClass.createRow(row11);
    Term addressRow2Set = tableClass.createRow(row2);

    // Insert row1 then row2 into an empty table
    Term addressTableWithRow1 = RelationalAlgebras::Insert(slv, tableClass, tableTerm, addressRow1Set);
    Term addressTableWithRow1Row2 = RelationalAlgebras::Insert(slv, tableClass, addressTableWithRow1, addressRow2Set);

    // Insert row2 then row1 into an empty table
    Term addressTableWithRow2 = RelationalAlgebras::Insert(slv, tableClass, tableTerm, addressRow2Set);
    Term addressTableWithRow2Row1 = RelationalAlgebras::Insert(slv, tableClass, addressTableWithRow2, addressRow1Set);

    // Check if finalTable1 is equal to finalTable2
    Term checkEquality = slv.mkTerm(Kind::EQUAL, {addressTableWithRow2Row1, addressTableWithRow1Row2});
    Term checkNotEmpty = slv.mkTerm(Kind::DISTINCT, {EmptyTable, tableTerm});
    Term checkNotDefault = slv.mkTerm(Kind::DISTINCT, {defaultRowSet, tableTerm});

    Term stateCommutivityForTwoInserts = slv.mkTerm(Kind::AND, {checkEquality, checkNotEmpty, checkNotDefault});

    return stateCommutivityForTwoInserts;
}
