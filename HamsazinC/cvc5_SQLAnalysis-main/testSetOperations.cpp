#include <iostream>

#include <cvc5/cvc5.h>
#include "header/Table.h"
#include "header/TableInfo.h"
#include "header/RelationalAlgebras.h"
#include "header/RelationalProver.h"

using namespace std;
using namespace cvc5;

int temp20() {
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


    map<string, pair<Table, Term>> myState;

    myState["address"] = make_pair(addressTable, addressTableTerm);
    myState["zip-code"] = make_pair(zipCodeTable, zipCodeTableTerm);


    Term checkAddressNotEmpty = slv.mkTerm(Kind::DISTINCT, {emptyAddressBag, addressTableTerm});
    Term checkZipCodeNotEmpty = slv.mkTerm(Kind::DISTINCT, {emptyZipCodeBag, zipCodeTableTerm});
    Term checkInitialState = slv.mkTerm(Kind::AND, {checkAddressNotEmpty, checkZipCodeNotEmpty});
    slv.assertFormula(checkInitialState);

    ProjectionReturn temp = RelationalAlgebras::Product2(slv, addressTable, zipCodeTable, addressTableTerm, zipCodeTableTerm);
    Term product = temp.setComprehension;
    cout << product << endl;
    cout << product.getSort() << endl;

    Term addressRow = addressTable.createRow({1, "ad", "aa", "USA", "fg"});
    Term addressRow2 = addressTable.createRow({2, "ad", "aa", "china", "temp"});
    Term zipcodeRow = zipCodeTable.createRow({"USA", "dd", "ww"});
    Term zipcodeRow2 = zipCodeTable.createRow({"china", "gg", "pp"});
    slv.assertFormula(slv.mkTerm(Kind::SET_MEMBER, {addressRow, addressTableTerm}));
    slv.assertFormula(slv.mkTerm(Kind::SET_MEMBER, {addressRow2, addressTableTerm}));
    slv.assertFormula(slv.mkTerm(Kind::SET_MEMBER, {zipcodeRow, zipCodeTableTerm}));
    slv.assertFormula(slv.mkTerm(Kind::SET_MEMBER, {zipcodeRow2, zipCodeTableTerm}));

    cout << addressRow << endl;
//    slv.assertFormula(slv.mkTerm(Kind::SET_IS_SINGLETON, {product}));
    Result result = slv.checkSat();
    cout << "cvc5 reports: " << " " << " is "
         << result << "." << endl;
    if (result.isSat()) {
        cout << "For instance, " <<  slv.getValue(addressTableTerm) << " is a member." << endl;
        cout << "For instance, " <<  slv.getValue(product) << " is a member." << endl;
    }

    return 0;
}
