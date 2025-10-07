#include <utility>

#include "../header/RelationalProver.h"


Term RelationalProver::referentialIntegrity(Solver& slv, map<string, pair<Table, Term>> initialState) {

    vector<Term> refIntegrities;
    for(const auto& entry : initialState) {
//        std::cout << "Key: " << entry.first << ", Value: " << entry.second.second << std::endl;

        Table tableA = initialState[entry.first].first;
        Term tableATerm = initialState[entry.first].second;
        vector<string> tableAColumns = tableA.getTableColumnNames();
        auto tableRefIntegrity = TableInfo::getReferentialIntegrityInfo(entry.first);

        for(const auto& entry2 : tableRefIntegrity) {
//            std::cout << "Key2: " << entry2.first[1] << ", Value2: " << entry2.second[1] << std::endl;

            // Check if the foreign table exist in the initial state
            if(initialState.find(entry2.first[1]) != initialState.end()) {

                Table tableB = initialState[entry2.first[1]].first;
                Term tableBTerm = initialState[entry2.first[1]].second;
                vector<string> tableBColumns = tableB.getTableColumnNames();

                //Check if the foreign column exist in tableA and tableB
                if(find(tableAColumns.begin(), tableAColumns.end(), entry2.second[0]) == tableAColumns.end()) {
                    continue;
                }
                if(find(tableBColumns.begin(), tableBColumns.end(), entry2.second[1]) == tableBColumns.end()) {
                    continue;
                }

                ProjectionReturn resultA = RelationalAlgebras::Project(
                        slv,
                        tableA,
                        tableATerm,
                        {entry2.second[0]}
                );

                ProjectionReturn resultB = RelationalAlgebras::Project(
                        slv,
                        tableB,
                        tableBTerm,
                        {entry2.second[1]}
                );

//                Term refIntegrity = slv.mkTerm(Kind::BAG_SUBBAG, {resultA.setComprehension, resultB.setComprehension});
                Term refIntegrity = slv.mkTerm(Kind::SET_SUBSET, {resultA.setComprehension, resultB.setComprehension});

//                cout << refIntegrity << endl;
//                cout << resultA.setComprehension << endl;
//                cout << tableATerm << endl;
//                cout << resultB.setComprehension << endl;
//                cout << tableBTerm << endl<< endl<< endl;
                refIntegrities.push_back(refIntegrity);
            }
        }
    }

    Term combinedAssertion;
    combinedAssertion = refIntegrities[0];
    for (size_t i = 1; i < refIntegrities.size(); ++i) {
        combinedAssertion = slv.mkTerm(Kind::AND, {combinedAssertion, refIntegrities[i]});
    }

    return combinedAssertion;
}

Term RelationalProver::StateCommutativeCheck(Solver &slv, const map<string, pair<Table, Term>>& state1, map<string, pair<Table, Term>> state2) {

    std::vector<Term> equalities;
    for (const auto& entry : state1) {
        const std::string& key1 = entry.first;
        const Term& term1 = entry.second.second;
        const Term& term2 = state2[key1].second;

        std::cout << "Key: " << key1 << "-----> Term1: " << term1 << "------> Term2: " << term2 << std::endl;
        Term equality = slv.mkTerm(Kind::EQUAL, {term1, term2});
        equalities.push_back(equality);
//        slv.assertFormula(slv.mkTerm(Kind::EQUAL, {term1, term2}));
    }
    Term combinedAssertion;

    combinedAssertion = equalities[0];
    for (size_t i = 1; i < equalities.size(); ++i) {
        combinedAssertion = slv.mkTerm(Kind::AND, {combinedAssertion, equalities[i]});
    }
    // Apply negation
    combinedAssertion = slv.mkTerm(Kind::NOT, {combinedAssertion});

    return combinedAssertion;
}

Term RelationalProver::PRCommutativeCheck(Solver &slv, const map<string, pair<Table, Term>>& state1, const map<string, pair<Table, Term>>& state2) {

    Term refIntegrity1 = referentialIntegrity(slv, state1);
    Term refIntegrity2 = referentialIntegrity(slv, state2);

    Term testPR = slv.mkTerm(Kind::IMPLIES, {refIntegrity1, refIntegrity2});
//    testPR = slv.mkTerm(Kind::AND, {testPR, refIntegrity1});

    // Apply negation
    testPR = slv.mkTerm(Kind::NOT, {testPR});

    return testPR;
//    return slv.mkTerm(Kind::AND, {slv.mkTerm(Kind::NOT, {refIntegrity2}), refIntegrity1});
}

Term RelationalProver::InvariantSufficientCheck(Solver &slv, map<string, pair<Table, Term>> initialState, map<string, pair<Table, Term>> state1) {
    Term refIntegrity1 = referentialIntegrity(slv, std::move(initialState));
    Term refIntegrity2 = referentialIntegrity(slv, std::move(state1));
    Term testPR = slv.mkTerm(Kind::IMPLIES, {refIntegrity1, refIntegrity2});
    // Apply negation
    Term negatedCombinedRefIntegrity = slv.mkTerm(Kind::NOT, {testPR});

    return negatedCombinedRefIntegrity;
}