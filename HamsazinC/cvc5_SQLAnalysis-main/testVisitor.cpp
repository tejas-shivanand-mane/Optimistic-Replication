#include "header/VisitorNodes.h"
#include "header/SqlQueries.h"


int main() {
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

    std::map<std::string, std::vector<std::string>> kwargs1 = {
            {"selectPredicate", {"ad_ctry", "eq", "USA"}}
    };
    std::map<std::string, std::vector<std::string>> kwargs2 = {
//            {"projectColumns", {"ad_id", "ad_ctry", "ad_zc_code"}},
//            {"projectColumns", {"zc_code"}},
            {"selectPredicate", {"ad_ctry", "eq", "Canada"}},
            {"updateRow", {"ad_ctry", "USA"}}
    };
    std::map<std::string, std::vector<std::string>> kwargs3 = {
//            {"projectColumns", {"ad_ctry", "ad_id", "ad_zc_code"}},
            {"selectPredicate", {"zc_code", "eq", "d123ad"}},
            {"updateRow", {"zc_code", "dd456aa"}}
    };
    std::map<std::string, std::vector<std::string>> kwargs4 = {
//            {"projectColumns", {"ad_ctry", "ad_id", "ad_zc_code"}},
            {"selectPredicate", {"ad_ctry", "eq", "USA"}},
            {"updateRow", {"ad_ctry", "China"}}
    };
    std::map<std::string, std::vector<std::string>> kwargs42 = {
//            {"projectColumns", {"ad_ctry", "ad_id", "ad_zc_code"}},
            {"selectPredicate", {"ad_zc_code", "eq", "a123d"}},
            {"updateRow", {"ad_zc_code", "f55d5"}}
    };
    std::map<std::string, std::vector<std::string>> kwargs7 = {
            {"insertRow", {"1", "ad", "aa", "ss", "fg"}},
//            {"insertRow", {"ad", "aa", "ss"}},
//            {"deleteRow", {"1", "adf", "bc", "Canada", "USA"}},
    };
    std::map<std::string, std::vector<std::string>> kwargs5 = {
//            {"insertRow", {"1", "ad", "bc", "Canada", "USA"}},
//            {"insertRow", {"bc", "Canada", "USA"}},
//            {"deleteRow", {"1", "adf", "bc", "Canada", "USA"}},
            {"deleteRow", {"bc", "Canada", "USA"}},
    };
    std::map<std::string, std::vector<std::string>> kwargs6 = {
//            {"insertRow", {"1", "ff", "dd", "China", "Canada"}},
//            {"insertRow", {"ad", "bc", "Canada"}},
//            {"deleteRow", {"1", "ad", "bc", "Canada", "Canada"}},
            {"deleteRow", {"ad", "bc", "Canada"}},
    };

    std::map<std::string, std::vector<std::string>> kwargs10 = {
            {"joinColumns", {"ad_zc_code", "zc_code"}},
    };

    std::map<std::string, std::vector<std::string>> kwargs11 = {
            {"selectPredicate", {"ad_ctry", "eq", "L"}},
            {"updateRow", {"ad_ctry", "China"}},
            {"joinColumns", {"ad_zc_code", "zc_code"}},
    };

    vector<string> tableNames = {"address", "zip-code"};
    vector<string> tableNames2 = {"zip-code", "address"};

    cout << "HEREEEEEEEE! " << endl;
    Context query1 = SqlQueries::JoinAndUpdate(slv, myState, tableNames, kwargs11);
    cout << "HEREEEEEEEE! " << endl;

//    Context query1 = SqlQueries::Product(slv, myState, tableNames, kwargs10);
//    Context query1 = SqlQueries::Select(slv, myState, tableNames, kwargs1);
//    Context query1 = SqlQueries::Project(slv, myState, tableNames2, kwargs2);
//    Context query1 = SqlQueries::Update(slv, myState, tableNames, kwargs2);
//    Context query1 = SqlQueries::Insert(slv, myState, tableNames, kwargs7);
//    Context query12 = SqlQueries::Insert(slv, query1.getState(), tableNames2, kwargs6);
//    Context query12 = SqlQueries::Update(slv, myState, tableNames, kwargs4);
//    Context query12 = SqlQueries::Delete(slv, query1.getState(), tableNames2, kwargs6);
//    Context query12 = SqlQueries::Delete(slv, query1.getState(), tableNames, kwargs5);
//    Context query12 = SqlQueries::Delete(slv, query1.getState(), tableNames, kwargs5);

//    Context query2 = SqlQueries::Delete(slv, myState, tableNames2, kwargs6);
//    Context query2 = SqlQueries::Insert(slv, myState, tableNames2, kwargs6);
//    Context query2 = SqlQueries::Update(slv, myState, tableNames, kwargs4);
//    Context query21 = SqlQueries::Insert(slv, query2.getState(), tableNames, kwargs7);
    Context query21 = SqlQueries::Update(slv, myState, tableNames2, kwargs3);
    Context query2 = SqlQueries::Product(slv, query21.getState(), tableNames, kwargs10);
//    Context query21 = SqlQueries::Delete(slv, query2.getState(), tableNames2, kwargs6);
//    Context query2 = SqlQueries::Project(slv, myState, tableNames2, kwargs2);
//    Context query2 = SqlQueries::Select(slv, myState, tableNames, kwargs1);
//    Context query21 = SqlQueries::Select(slv, query2.getState(), tableNames, kwargs1);
//    Context query21 = SqlQueries::Insert(slv, query2.getState(), tableNames, kwargs7);

//    Term stateCheck = RelationalProver::StateCommutativeCheck(slv, myState, query21.getState());
    Term PRCheck = RelationalProver::PRCommutativeCheck(slv, myState, query21.getState());
//    Term ICheck = RelationalProver::InvariantSufficientCheck(slv, myState, query1.getState());

    Term refInitial = RelationalProver::referentialIntegrity(slv, myState);
    slv.assertFormula(refInitial);
//    cout << stateCheck << endl;
//    slv.assertFormula(stateCheck);
    cout << PRCheck << endl;
    slv.assertFormula(PRCheck);
//    cout << ICheck << endl;
//    slv.assertFormula(ICheck);
    slv.assertFormula(checkInitialState);

    Result result = slv.checkSat();
    cout << "cvc5 reports: " << " " << " is "
         << result << "." << endl;
    if (result.isSat()) {
//        cout << "Results State: " << slv.getValue(stateCheck) << endl;
        cout << "Results PR: " << slv.getValue(PRCheck) << endl;
//        cout << "Results I check: " << slv.getValue(ICheck) << endl;
        cout << "Results Inital Refrensial: " << slv.getValue(refInitial) << endl;
        cout << "Initial State: " << endl;
        for(const auto& tablePair : myState){
            cout << "For instance, " << tablePair.first  << " " <<  slv.getValue(tablePair.second.second) << " is a member." << endl;
        }
        cout << "Query1 State: " << endl;
        for(const auto& tablePair : query1.getState()){
            cout << "For instance, " << tablePair.first  << " " <<  slv.getValue(tablePair.second.second) << " is a member." << endl;
        }
        cout << "Query2 State: " << endl;
        for(const auto& tablePair : query21.getState()){
            cout << "For instance, " << tablePair.first  << " " <<  slv.getValue(tablePair.second.second) << " is a member." << endl;
        }
    }


    return 0;
}
