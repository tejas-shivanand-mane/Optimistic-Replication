#include "../header/RelationalAlgebras.h"

Term RelationalAlgebras::Insert(Solver &slv, Table table, const Term &tableSet, const RowData &newRow) {
    Term newRowSet = table.createRow(newRow);
//    Term bagToRemove = slv.mkTerm(Kind::BAG_MAKE, {newRowSet, slv.mkInteger(1)});
//    Term insertedRow = slv.mkTerm(Kind::BAG_UNION_DISJOINT, {tableSet, bagToRemove});
    Term bagToRemove = slv.mkTerm(Kind::SET_SINGLETON, {newRowSet});
    Term insertedRow = slv.mkTerm(Kind::SET_UNION, {tableSet, bagToRemove});
    return insertedRow;
}

Term RelationalAlgebras::Delete(Solver &slv, Table table, const Term &tableSet, const RowData &rowToDelete) {
    Term newRowSet = table.createRow(rowToDelete);
//    Term bagToRemove = slv.mkTerm(Kind::BAG_MAKE, {newRowSet, slv.mkInteger(1)});
    Term bagToRemove = slv.mkTerm(Kind::SET_SINGLETON, {newRowSet});

    // Perform the bag difference remove operation
//    Term updatedTableSet = slv.mkTerm(Kind::BAG_DIFFERENCE_SUBTRACT, {tableSet, bagToRemove});
    Term updatedTableSet = slv.mkTerm(Kind::SET_MINUS, {tableSet, bagToRemove});

    return updatedTableSet;
}

Term RelationalAlgebras::Select(Solver &slv, Table table, const Term &tableSet, const std::string &columnName,
                                const std::variant<std::string, int> &value, const std::string &selectOperation) {
    // Define the sort of your row elements
//    Sort rowSort = table.getBagTableSort().getBagElementSort(); // The sort of your row elements
    Sort rowSort = table.getBagTableSort().getSetElementSort(); // The sort of your row elements

    // Create a bound variable to represent each element in the set
    Term boundVarElementSort = slv.mkVar(rowSort, "row");

    // Create a variable list for the comprehension
    std::vector<Term> vars = {boundVarElementSort};
    Term varList = slv.mkTerm(Kind::VARIABLE_LIST, vars);

    Term predicate = selectPredicate(slv, table, boundVarElementSort, columnName, value, selectOperation);

    Term lambda = slv.mkTerm(Kind::LAMBDA, {varList, predicate});
//    Term filteredSet = slv.mkTerm(Kind::BAG_FILTER, {lambda, tableSet});
    Term filteredSet = slv.mkTerm(Kind::SET_FILTER, {lambda, tableSet});
    // Return the filtered set
    return filteredSet;
}

Term RelationalAlgebras::selectPredicate(Solver &slv, Table table, const Term &boundVar, const std::string &columnName,
                                         std::variant<std::string, int> value, const std::string &selectOperation) {

    // Assuming 'add_cntry' is a field in your row and you have a method to access it
    int columnIndex = table.getColumnIndexByName(columnName);
    Term addCntryField = table.accessFieldByIndex(boundVar, columnIndex);
    Term valueTerm;
    // Create a string term for "value"
    if (std::holds_alternative<int>(value)) {
        valueTerm = slv.mkInteger(std::get<int>(value));
    } else if (std::holds_alternative<std::string>(value)) {
        valueTerm = slv.mkString(std::get<std::string>(value));
    }

    // Create the predicate (condition) for the comprehension
    Term predicate;
    if (selectOperation == "eq") {
        predicate = slv.mkTerm(Kind::EQUAL, {addCntryField, valueTerm});
    } else if (selectOperation == "lt") {
        predicate = slv.mkTerm(Kind::LT, {addCntryField, valueTerm});
    } else if (selectOperation == "lte") {
        predicate = slv.mkTerm(Kind::LEQ, {addCntryField, valueTerm});
    } else if (selectOperation == "gt") {
        predicate = slv.mkTerm(Kind::GT, {addCntryField, valueTerm});
    } else if (selectOperation == "gte") {
        predicate = slv.mkTerm(Kind::GEQ, {addCntryField, valueTerm});
    } else if (selectOperation == "ne") {
        predicate = slv.mkTerm(Kind::DISTINCT, {addCntryField, valueTerm});
    }
    else {
        throw std::runtime_error("No operation for " + selectOperation);
    }
    return predicate;
}

Term TransformRow(Solver &slv, const Term &originalRow, Table originalTable,
                                      Table newTable, const std::vector<std::string> &desiredColumns) {
    // List to hold the terms for each desired column value
    std::vector<Term> newColumnValues;

//    newColumnValues.push_back(newTable.getTableSort().getDatatype()[0].getTerm());
// Bag
    newColumnValues.push_back(newTable.getTableSort().getSetElementSort().getDatatype()[0].getTerm());

    // Extract the values of the desired columns
    for (const auto &columnName: desiredColumns) {
        Term columnValue = originalTable.accessField(originalRow, columnName);
        newColumnValues.push_back(columnValue);
    }

    // Create a new row with these values for the new table
    // Assuming the new table's datatype constructor takes values in the same order as desiredColumns
    Term newRow = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, {newColumnValues});

    return newRow;
}

ProjectionReturn RelationalAlgebras::Project(Solver &slv, Table originalTable, const Term &originalSet,
                                             const std::vector<std::string> &desiredColumns) {
    // Step 1: Create a new table with the desired columns
    Table newTable = Table(slv, originalTable, desiredColumns);
    Sort testSort = newTable.getBagTableSort();

    // Indices of columns to project
    vector<uint32_t> indices = originalTable.getIndicesOfColumns(desiredColumns);

    // Create the TABLE_PROJECT operator
//    Op tableProjectOp = slv.mkOp(Kind::TABLE_PROJECT, indices);
    // Create the term for table projection
//    Term tableProjectTerm = slv.mkTerm(tableProjectOp, {originalSet});

    Term boundVar = slv.mkVar(originalSet.getSort().getSetElementSort(), "row");
    std::vector<Term> vars = {boundVar};
    Term varList = slv.mkTerm(Kind::VARIABLE_LIST, vars);
    // Define the lambda function for the transformation
    Term transformation = TransformRow(slv, boundVar, originalTable, newTable, desiredColumns);
    Term lambda = slv.mkTerm(Kind::LAMBDA, {varList, transformation});

    // Apply SET_FILTER to transform each row according to the lambda function
    Term filteredSet = slv.mkTerm(Kind::SET_MAP, {lambda, originalSet});


//    return ProjectionReturn{tableProjectTerm, newTable};
    return ProjectionReturn{filteredSet, newTable};
}

ProjectionReturn RelationalAlgebras::Product(Solver &slv, Table tableA, Table tableB, const Term &setA, const Term &setB, string columTableA, string columTableB) {
    // Convert Bags
//    Term product = slv.mkTerm(Kind::TABLE_PRODUCT, {setA, setB});

    // Prepare datatype for the joined table
    std::string joinedTableName = tableA.getTableName() + "_join_" + tableB.getTableName();
    cout << joinedTableName << endl;
    DatatypeDecl joinedTableDatatype = slv.mkDatatypeDecl(joinedTableName);
    DatatypeConstructorDecl joinedTableConstructor = slv.mkDatatypeConstructorDecl("make-" + joinedTableName);
    // Define the new datatype by combining the datatypes from both tables
    // add columns from the first table
    const Datatype &firstTableDatatype = tableA.getTableSort().getSetElementSort().getDatatype();

    const DatatypeConstructor &firstTableConstructor = firstTableDatatype[0];
//    const Datatype &secondTableDatatype = tableB.getTableSort().getDatatype();
    const Datatype &secondTableDatatype = tableB.getTableSort().getSetElementSort().getDatatype();
    const DatatypeConstructor &secondTableConstructor = secondTableDatatype[0];
    // add columns from the first table
    vector<Sort> tableSorts;
    for (size_t i = 0; i < firstTableConstructor.getNumSelectors(); ++i) {
        std::string columnName = firstTableConstructor[i].getName();
        Sort columnSort = firstTableConstructor[i].getCodomainSort();
        tableSorts.push_back(columnSort);
    }

    // add columns from the second table, excluding the join column
    for (size_t i = 0; i < secondTableConstructor.getNumSelectors(); ++i) {
        std::string columnName = secondTableConstructor[i].getName();
        Sort columnSort = secondTableConstructor[i].getCodomainSort();

        tableSorts.push_back(columnSort);
    }

    Sort tupleSort = slv.mkTupleSort(tableSorts);
//    Sort joinedSort = slv.mkBagSort(tupleSort);
    Sort joinedSort = slv.mkSetSort(tupleSort);

    // Create joined tables
    std::vector<std::pair<std::string, cvc5::Sort>> columnNamesB = tableB.getColumnNames();
    std::vector<std::pair<std::string, cvc5::Sort>> concatenated = tableA.getColumnNames();
    // Insert all elements of the second vector at the end of the first
    concatenated.insert(concatenated.end(), columnNamesB.begin(), columnNamesB.end());

    Table joinedTable(slv, joinedTableName, joinedSort, concatenated);

//    Term boundVarA = slv.mkVar(setA.getSort().getSetElementSort(), "rowA");
//    std::vector<Term> varsA = {boundVarA};
//    Term varListA = slv.mkTerm(Kind::VARIABLE_LIST, varsA);
//
//    Term boundVarB = slv.mkVar(setB.getSort().getSetElementSort(), "rowB");
//    std::vector<Term> varsB = {boundVarB};
//    Term varListB = slv.mkTerm(Kind::VARIABLE_LIST, varsB);

//    int columnIndexA = tableA.getColumnIndexByName("ad_zc_code");
//    Term tableAField = tableA.accessFieldByIndex(boundVarA, columnIndexA);
//
//    int columnIndexB = tableB.getColumnIndexByName("zc_code");
//    Term tableBField = tableB.accessFieldByIndex(boundVarB, columnIndexB);
//
//    Term tempA = slv.mkTerm(Kind::BAG_FROM_SET, {setA});
//    Term tempB = slv.mkTerm(Kind::BAG_FROM_SET, {setB});
//    Term product = slv.mkTerm(Kind::TABLE_PRODUCT, {tempA, tempB});
//    Term tempProduct = slv.mkTerm(Kind::BAG_TO_SET, {product});
//
    //RELATION_JOIN or RELATION_PRODUCT or RELATION_JoinIMG (non-binary relation?)
    Term product = slv.mkTerm(Kind::RELATION_PRODUCT, {setA, setB});

    Sort rowSort = product.getSort().getSetElementSort(); // The sort of your row elements
    Term boundVar = slv.mkVar(rowSort, "row");
    std::vector<Term> vars = {boundVar};
    Term varList = slv.mkTerm(Kind::VARIABLE_LIST, vars);

    int columnIndexA = tableA.getColumnIndexByName(columTableA);
    Term joinFieldA = joinedTable.accessFieldByIndex(boundVar, columnIndexA);

    int columnIndexB = tableB.getColumnIndexByName(columTableB);
    Term joinFieldB = joinedTable.accessFieldByIndex(boundVar, columnIndexA + columnIndexB);

    Term lambda = slv.mkTerm(Kind::LAMBDA, {varList, slv.mkTerm(Kind::EQUAL, {joinFieldA, joinFieldB})});
//    Term filteredSet = slv.mkTerm(Kind::BAG_FILTER, {lambda, tableSet});
    Term filteredSet = slv.mkTerm(Kind::SET_FILTER, {lambda, product});



    return ProjectionReturn{filteredSet, joinedTable};
//    s_1 . s_2 =
//            map (\ t_1. map (\ t_2.  if t_1.c_1 = t_2.c_2 then t1.t2 else \bot) s_2) s_1
//    std::vector<Term> newTupleElements;
//    DatatypeConstructor ctorA = setA.getSort().getSetElementSort().getDatatype()[0];
//    for (int i = 0; i < 5; ++i) {
//        const DatatypeSelector& selector = ctorA[i];
//        newTupleElements.push_back(slv.mkTerm(Kind::APPLY_SELECTOR, {selector.getTerm(), boundVarA}));
//    }
//    DatatypeConstructor ctorB = setB.getSort().getSetElementSort().getDatatype()[0];
//    for (int i = 0; i < 3; ++i) {
//        const DatatypeSelector& selector = ctorB[i];
//        newTupleElements.push_back(slv.mkTerm(Kind::APPLY_SELECTOR, {selector.getTerm(), boundVarB}));
//    }
//    cout << newTupleElements << endl;
//
//    Term concatenatedTuple = slv.mkTuple(newTupleElements);
//
//    Term lambdaInner = slv.mkTerm(Kind::LAMBDA, {varListB, concatenatedTuple});
//    // Apply the first map
//    Term firstMap = slv.mkTerm(Kind::SET_MAP, {lambdaInner, setB});
//    cout << firstMap << endl;
//    // Define a lambda for the outer map
//    // This lambda applies the first map to each element of setA
//    Term lambdaOuter = slv.mkTerm(Kind::LAMBDA, {varListA, firstMap});
//    cout << lambdaOuter << endl;
//    // Apply the outer map to create the Cartesian product
//    Term cartesianProduct = slv.mkTerm(Kind::SET_MAP, {lambdaOuter, setA});
//
//    cout << cartesianProduct << endl;
//    cout << cartesianProduct.getSort() << endl;
//
//
//    Sort setOfSetsSort = cartesianProduct.getSort();
//    Sort innerSetSort = setOfSetsSort.getSetElementSort();
//    Term boundVarAB = slv.mkVar(innerSetSort, "rowAB");
//    std::vector<Term> varsAB = {boundVarAB};
//    Term varListAB = slv.mkTerm(Kind::VARIABLE_LIST, varsAB);
//
//    Term emptySet = slv.mkEmptySet(cartesianProduct.getSort());
//
//    // Create a lambda function for the folding operation
//    Term lambdaVarSet = slv.mkVar(innerSetSort, "innerSet"); // Variable for the inner set
//    Term lambdaVarAccum = slv.mkVar(innerSetSort, "accumSet"); // Variable for the accumulating set
//    cout << lambdaVarSet.getSort() << endl;
//
//    // Define the union operation as the folding function
//    Term unionExpr = slv.mkTerm(Kind::SET_UNION, {boundVarAB, boundVarAB});
//    Term lambdaUnion = slv.mkTerm(Kind::LAMBDA, {varListAB, unionExpr});
//    cout << endl << endl;
//    cout << lambdaUnion << endl;

    // Apply the SET_FOLD operation
//    Term flattenedSet = slv.mkTerm(Kind::SET_FOLD, {lambdaUnion, emptySet, cartesianProduct});

//    std::cout << "Flattened Set: \n" << flattenedSet << std::endl;


//    // Define the lambda function for the transformation
//    Term lambdaB = slv.mkTerm(Kind::LAMBDA, {varListB, boundVarB});
//    // Apply the first map
//    Term mappedB = slv.mkTerm(Kind::SET_MAP, {lambdaB, setB});
//    // Define the nested map with join condition
//    cout << mappedB << endl;
//    Term joinCondition = slv.mkTerm(Kind::EQUAL, {tableAField, tableBField});
////    Term joining = slv.mkTerm(Kind::ITE, joinCondition,
////                              slv.mkTerm(Kind::TUPLE, newTupleElements),
////                              slv.mkTerm(Kind::TUPLE, {}));
//    Term lambdaA = slv.mkTerm(Kind::LAMBDA, {varListA, joinCondition});
//    cout << lambdaA << endl;
//    // Apply the nested map
//    Term nestedMap = slv.mkTerm(Kind::SET_MAP, {lambdaA, mappedB});
//    Term nested = slv.mkTerm(Kind::SET_MAP, {lambdaA, slv.mkTerm(Kind::SET_MAP, {lambdaB, setB})});


//    Term joinCondition = slv.mkTerm(Kind::EQUAL, {tableAField, tableBField});
//    std::vector<Term> newTupleElements;
//    // ... Fill newTupleElements appropriately ...
//    Term newTuple = slv.mkTerm(Kind::ITE, joinCondition,
//                               slv.mkTerm(Kind::TUPLE, newTupleElements),
//                               slv.mkTerm(Kind::TUPLE, {})); // Empty tuple for else case
//    Term lambdaInner = slv.mkTerm(Kind::LAMBDA, lambdaVarB, newTuple);
//
//    // Apply the first map
//    Term firstMap = slv.mkTerm(Kind::SET_MAP, lambdaInner, setB);
//
//    // Define a lambda for the outer map
//    // This lambda applies the first map to each element of setA
//    Term lambdaOuter = slv.mkTerm(Kind::LAMBDA, lambdaVarA, firstMap);
//
//    // Apply the outer map
//    Term result = slv.mkTerm(Kind::SET_MAP, lambdaOuter, setA);
}

ProjectionReturn RelationalAlgebras::Product2(Solver &slv, Table tableA, Table tableB, const Term &setA, const Term &setB) {
    // Convert Bags
    Term product = slv.mkTerm(Kind::TABLE_PRODUCT, {setA, setB});
    // Prepare datatype for the joined table
    std::string joinedTableName = tableA.getTableName() + "_join_" + tableB.getTableName();
    DatatypeDecl joinedTableDatatype = slv.mkDatatypeDecl(joinedTableName);
    DatatypeConstructorDecl joinedTableConstructor = slv.mkDatatypeConstructorDecl("make-" + joinedTableName);
    // Define the new datatype by combining the datatypes from both tables
    // add columns from the first table
    const Datatype &firstTableDatatype = tableA.getTableSort().getDatatype();
    const DatatypeConstructor &firstTableConstructor = firstTableDatatype[0];

//    const Datatype &secondTableDatatype = tableB.getTableSort().getDatatype();
    const Datatype &secondTableDatatype = tableB.getTableSort().getDatatype();
    const DatatypeConstructor &secondTableConstructor = secondTableDatatype[0];
    // add columns from the first table
    vector<Sort> tableSorts;
    for (size_t i = 0; i < firstTableConstructor.getNumSelectors(); ++i) {
        std::string columnName = firstTableConstructor[i].getName();
        Sort columnSort = firstTableConstructor[i].getCodomainSort();
        tableSorts.push_back(columnSort);
    }

    // add columns from the second table, excluding the join column
    for (size_t i = 0; i < secondTableConstructor.getNumSelectors(); ++i) {
        std::string columnName = secondTableConstructor[i].getName();
        Sort columnSort = secondTableConstructor[i].getCodomainSort();

        tableSorts.push_back(columnSort);
    }

    Sort tupleSort = slv.mkTupleSort(tableSorts);
    Sort joinedSort = slv.mkBagSort(tupleSort);

    // Create joined tables
    std::vector<std::pair<std::string, cvc5::Sort>> columnNamesB = tableB.getColumnNames();
    std::vector<std::pair<std::string, cvc5::Sort>> concatenated = tableA.getColumnNames();
    // Insert all elements of the second vector at the end of the first
    concatenated.insert(concatenated.end(), columnNamesB.begin(), columnNamesB.end());

    Table joinedTable(slv, joinedTableName, joinedSort, concatenated);
    return ProjectionReturn{product, joinedTable};
}

Term RelationalAlgebras::Aggregation(Solver &slv, const Table& table, const Term &tableTerm, uint32_t columnIndex) {
    // Assuming the column to sum is an integer
    Sort intSort = {slv.getIntegerSort()};
    Sort intSorts = slv.mkTupleSort({slv.getIntegerSort()});

    // Get the tuple sort representing a row in the table
    Sort tupleSort = tableTerm.getSort().getBagElementSort();
    // Define the function type for summation: (-> (Tuple ...) Int Int)
    Sort functionType = slv.mkFunctionSort({tupleSort, intSort}, intSort);

    // Create a function symbol for the summation
    Term sumFunction = slv.mkConst(functionType, "integerFunction");

    // Initial value for sum
    Term zero = slv.mkInteger(0);

    // Define the aggregate operation with the index of the column to sum
    std::vector<uint32_t> indices = {columnIndex};
    Op aggregateOp = slv.mkOp(Kind::TABLE_AGGREGATE, indices);

    // Create the aggregation term
    Term aggregateTerm = slv.mkTerm(aggregateOp, {sumFunction, zero, tableTerm});
    return aggregateTerm;
}

Term RelationalAlgebras::Update(Solver &slv, Table table, const Term &tableSet, const std::string &columnName,
                                const std::variant<std::string, int> &value, const std::string &selectOperation, const pair<string, variant<std::string, int>> &updateValue) {

    std::vector<std::string> desiredColumns = (columnName == updateValue.first) ? vector<string>{columnName} : vector<string>{columnName, updateValue.first};

    Term selectedRows =  Select(slv, table, tableSet, columnName, value, selectOperation);
//    ProjectionReturn projectedReturn = Project(slv, table, selectedRows, desiredColumns);

//    Term originalWithoutSelected = slv.mkTerm(Kind::BAG_DIFFERENCE_REMOVE, {tableSet, selectedRows});
    Term originalWithoutSelected = slv.mkTerm(Kind::SET_MINUS, {tableSet, selectedRows});

//    cout << projectedReturn.setComprehension << "  ----> " << selectedRows.getSort() << "    ------    " << endl;

//    Term lambdaVar = slv.mkVar(table.getBagTableSort().getBagElementSort(), "row");
    Term lambdaVar = slv.mkVar(table.getBagTableSort().getSetElementSort(), "row");
    std::vector<Term> vars = {lambdaVar};
    Term varList = slv.mkTerm(Kind::VARIABLE_LIST, vars);

    Term updatedColumnValue = holds_alternative<int>(updateValue.second) ? slv.mkInteger(get<int>(updateValue.second)) : slv.mkString(get<string>(updateValue.second)); // The new value for the column
    std::vector<Term> tupleArgs;
    for (const auto& column : table.getColumnNames()) {
        if (column.first == updateValue.first) {
            tupleArgs.push_back(updatedColumnValue);
        } else {
            tupleArgs.push_back(table.accessField(lambdaVar, column.first));
        }
    }
    Term TupleVars = slv.mkTuple(tupleArgs);
    Term lambdaFunc = slv.mkTerm(Kind::LAMBDA, {varList, TupleVars});
//    Term updatedRows = slv.mkTerm(Kind::BAG_MAP, {lambdaFunc, selectedRows});
    Term updatedRows = slv.mkTerm(Kind::SET_MAP, {lambdaFunc, selectedRows});

//    Term updatedTableSet = slv.mkTerm(Kind::BAG_UNION_DISJOINT, {originalWithoutSelected, updatedRows});
    Term updatedTableSet = slv.mkTerm(Kind::SET_UNION, {originalWithoutSelected, updatedRows});
    return updatedTableSet;
}
