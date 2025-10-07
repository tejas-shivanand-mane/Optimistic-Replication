#include "header/VisitorNodes.h"
#include "header/SqlQueries.h"
#include "header/RelationalProver.h"

Term refIntegrity(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const vector<string>& columnNames) {
    const string& tableAName = tableNames[0];
    const string& tableBName = tableNames[1];

    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;

    Table tableB = initialState[tableBName].first;
    Term tableBTerm = initialState[tableBName].second;


    ProjectionReturn resultA = RelationalAlgebras::Project(
            slv,
            tableA,
            tableATerm,
            {columnNames[0]}
    );

    ProjectionReturn resultB = RelationalAlgebras::Project(
            slv,
            tableB,
            tableBTerm,
            {columnNames[1]}
    );

    Term refIntegrity = slv.mkTerm(Kind::BAG_SUBBAG, {resultA.setComprehension, resultB.setComprehension});
    return refIntegrity;
}

int test10() {
    // Initialize the Solver
    Solver slv;
    slv.setLogic("HO_ALL");
    slv.setOption("produce-models", "true");
    slv.setOption("output-language", "smt2");

    Table addressTable(slv, "address");
    Term addressTableTerm = addressTable.createBag();
    Term emptyAddressBag = addressTable.createEmptyBag();
    Term defaultAddressBag = addressTable.createDefaultBag();

    Table zipCodeTable(slv, "zip-code");
    Term zipCodeTableTerm = zipCodeTable.createBag();
    Term emptyZipCodeBag = zipCodeTable.createEmptyBag();
    Term defaultZipCodeBag = zipCodeTable.createDefaultBag();

    RowData addressRow1 = {5, "haha", "temp", "123", "USA"};
    RowData zipCodeRow1 = {"123", "haha", "temp"};
    RowData addressRow2 = {"123", "USA"};
    Term deletedTerm = RelationalAlgebras::Delete(slv, addressTable, addressTableTerm, addressRow1);
//    cout << deletedTerm << endl;

    map<string, pair<Table, Term>> myState;

    myState["address"] = make_pair(addressTable, addressTableTerm);
    myState["zip-code"] = make_pair(zipCodeTable, zipCodeTableTerm);


    Term checkAddressNotEmpty = slv.mkTerm(Kind::DISTINCT, {emptyAddressBag, addressTableTerm});
    Term checkZipCodeNotEmpty = slv.mkTerm(Kind::DISTINCT, {emptyZipCodeBag, zipCodeTableTerm});
    Term checkInitialState = slv.mkTerm(Kind::AND, {checkAddressNotEmpty, checkZipCodeNotEmpty});


    Context query1 = SqlQueries::Select(slv, myState, {"address", "zip-code"}, {{"selectPredicate", {"ad_ctry", "eq", "USA"}}} );
    auto state1 = query1.getState();
    Term refIntegrity1 = refIntegrity(slv, state1, {"address", "zip-code"}, {"ad_zc_code", "zc_code"});

    Context query2 = SqlQueries::Select(slv, myState, {"zip-code", "address"}, {{"selectPredicate", {"zc_code", "eq", "123"}}} );
    Context query21 = SqlQueries::Select(slv, myState, {"address", "zip-code"}, {{"selectPredicate", {"ad_ctry", "eq", "USA"}}} );
    auto state2 = query21.getState();
    Term refIntegrity2 = refIntegrity(slv, state2, {"address", "zip-code"}, {"ad_zc_code", "zc_code"});

//    state1["zip-code"].second = RelationalAlgebras::Delete(slv, state1["zip-code"].first, state1["zip-code"].second, zipCodeRow1);

    Term checkZipCodeNotEmpty1 = slv.mkTerm(Kind::DISTINCT, {emptyAddressBag, state2["address"].second});
    Term checkZipCodeNotEmpty2 = slv.mkTerm(Kind::DISTINCT, {emptyZipCodeBag, state2["zip-code"].second});


    Term refIntegrityImport = RelationalProver::PRCommutativeCheck(slv, myState, myState);
//    Term testPR = slv.mkTerm(Kind::IMPLIES, {refIntegrity1, refIntegrity2});
//    Term testPR = slv.mkTerm(Kind::AND, {refIntegrity1, slv.mkTerm(Kind::NOT, {refIntegrity2})});
    Term testPR = slv.mkTerm(Kind::OR, {slv.mkTerm(Kind::NOT, {refIntegrity1}), refIntegrity2});
    slv.assertFormula(refIntegrity1);
    slv.assertFormula(refIntegrity2);
    slv.assertFormula(checkInitialState);
    slv.assertFormula(checkZipCodeNotEmpty1);
    slv.assertFormula(checkZipCodeNotEmpty2);
    // Apply negation to the combined term
//    testPR = slv.mkTerm(Kind::NOT, {testPR});
    slv.assertFormula(testPR);
//    slv.assertFormula(slv.mkTerm(Kind::OR, {testRef1, negatedCombinedRefIntegrity}));
    // Assert the negated combined referential integrity
//    cout << "Ref integrity: " << refIntegrity1 << endl;
//    cout << "Ref integrity: " << refIntegrity2 << endl;
//    cout << "PR Commutivity: " << refIntegrityImport << endl;

    Result result = slv.checkSat();
    cout << "cvc5 reports: " << " " << " is "
         << result << "." << endl;
    if (result.isSat()) {
        cout << slv.getValue(refIntegrity1) << endl;
        cout << slv.getValue(refIntegrity2) << endl;
        cout << slv.getValue(testPR) << endl;
        cout << "For instance address Term, " << slv.getValue(addressTableTerm) << " ----> " << slv.getValue(state1["address"].second) << " is a member." << endl;
        cout << "For instance zipcode Term, " <<  slv.getValue(zipCodeTableTerm) << " ----> " << slv.getValue(state1["zip-code"].second) << " is a member." << endl;

        cout << "For instance address Term, " << slv.getValue(state2["address"].second) << " is a member." << endl;
        cout << "For instance zipcode Term, " <<  slv.getValue(state2["zip-code"].second) << " is a member." << endl;
    }
}