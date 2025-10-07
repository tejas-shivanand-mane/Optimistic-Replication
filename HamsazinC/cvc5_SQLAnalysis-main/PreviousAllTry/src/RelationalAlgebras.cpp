#include "../header/RelationalAlgebras.h"

Term RelationalAlgebras::Insert(Solver &slv, const Table &table, const Term &tableSet, const Term &newRowSet) {
    Term insertedRow = slv.mkTerm(Kind::SET_UNION, {tableSet, newRowSet});
    return insertedRow;
}

Term RelationalAlgebras::Select(Solver &slv, Table table, const Term &tableSet, const std::string &columnName,
                                const std::variant<std::string, int> &value, const std::string &selectOperation) {
    // Define the sort of your row elements
//    Sort rowSort = table.getTableSort(); // The sort of your row elements
    // Bag
    Sort rowSort = table.getBagTableSort().getBagElementSort(); // The sort of your row elements

    // Create a bound variable to represent each element in the set
//    Term boundVar = slv.mkVar(rowSort, "row");
    // Bag
    Term boundVarElementSort = slv.mkVar(rowSort, "row");

    // Create a variable list for the comprehension
//    std::vector<Term> vars = {boundVar};
    std::vector<Term> vars = {boundVarElementSort};
    Term varList = slv.mkTerm(Kind::VARIABLE_LIST, vars);

//    Term predicate = selectPredicate(slv, table, boundVar, columnName, value, selectOperation);
    Term predicate = selectPredicate(slv, table, boundVarElementSort, columnName, value, selectOperation);

    // Create the set comprehension term
//    Term setComprehension = slv.mkTerm(Kind::SET_COMPREHENSION, {varList, slv.mkTerm(Kind::AND,
//                                                                                     {slv.mkTerm(Kind::SET_MEMBER,
//                                                                                                 {boundVar, tableSet}),
//                                                                                      predicate}), boundVar});

    Term lambda = slv.mkTerm(Kind::LAMBDA, {varList, predicate});
//    Term filteredSet = slv.mkTerm(Kind::SET_FILTER, {lambda, tableSet});
    Term filteredSet = slv.mkTerm(Kind::BAG_FILTER, {lambda, tableSet});
    // Return the filtered set
    return filteredSet;
}

Term RelationalAlgebras::selectPredicate(Solver &slv, Table table, const Term &boundVar, const std::string &columnName,
                                         std::variant<std::string, int> value, const std::string &selectOperation) {

    // Assuming 'add_cntry' is a field in your row and you have a method to access it
//    Term addCntryField = table.accessField(boundVar, columnName);
    // Table
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
    return predicate;
}

ProjectionReturn RelationalAlgebras::Project(Solver &slv, Table originalTable, const Term &originalSet,
                                             const std::vector<std::string> &desiredColumns) {
    // Step 1: Create a new table with the desired columns
    Table newTable = Table(slv, originalTable, desiredColumns);
    Sort testSort = newTable.getBagTableSort();

    vector<Sort> tableSorts;
    for (const auto& item : newTable.getColumnNames()) {
        tableSorts.push_back(item.second);
    }
    // Indices of columns to project
    vector<uint32_t> indices = {3, 4};
    // Create the TABLE_PROJECT operator
    Op tableProjectOp = slv.mkOp(Kind::TABLE_PROJECT, indices);

    // Create the term for table projection
    Term tableProjectTerm = slv.mkTerm(tableProjectOp, {originalSet});
    return ProjectionReturn{tableProjectTerm, newTable};

    // Define the sort for the original and new rows
//    Sort originalRowSort = originalTable.getTableSort();
//    Sort newRowSort = newTable.getTableSort();
    // Bag
    Sort originalRowSort = originalTable.getBagTableSort();
    Sort newRowSort = newTable.getBagTableSort();

    // Create a bound variable to represent each element in the original set
    Term boundVar = slv.mkVar(originalRowSort, "row");

    // Create a variable list for the comprehension
    std::vector<Term> vars = {boundVar};
    Term varList = slv.mkTerm(Kind::VARIABLE_LIST, vars);
    // Step 2: Transform each row to match the new table's structure
    Term transformation = TransformRow(slv, boundVar, originalTable, newTable, desiredColumns);

    // Create the set comprehension term
//    Term setComprehension = slv.mkTerm(Kind::SET_COMPREHENSION,
//                                       {varList, slv.mkTerm(Kind::SET_MEMBER, {boundVar, originalSet}),
//                                        transformation});


    // Define the lambda function for the transformation
    Term lambda = slv.mkTerm(Kind::LAMBDA, {varList, transformation});

    // Apply SET_FILTER to transform each row according to the lambda function
//    Term filteredSet = slv.mkTerm(Kind::SET_MAP, {lambda, originalSet});

    // Bag
    Term filteredSet = slv.mkTerm(Kind::BAG_MAP, {lambda, originalSet});

    ProjectionReturn returnValue = {filteredSet, newTable};
    return returnValue;
}

Term RelationalAlgebras::TransformRow(Solver &slv, const Term &originalRow, Table originalTable,
                                      Table newTable, const std::vector<std::string> &desiredColumns) {
    // List to hold the terms for each desired column value
    std::vector<Term> newColumnValues;

//    newColumnValues.push_back(newTable.getTableSort().getDatatype()[0].getTerm());
// Bag
    newColumnValues.push_back(newTable.getBagTableSort().getDatatype()[0].getTerm());

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


Term combineRows(const Term &rowA, Table tableA, const Term &rowB, Table tableB, const Sort &joinedTableSort, Solver& slv) {
    // Assuming the first datatype constructor is used for each table row
    const DatatypeConstructor &constructorA = rowA.getSort().getDatatype()[0];
    const DatatypeConstructor &constructorB = rowB.getSort().getDatatype()[0];
    const DatatypeConstructor &joinedConstructor = joinedTableSort.getDatatype()[0];

    std::vector<Term> combinedValues;
    combinedValues.push_back(joinedConstructor.getTerm());

    // Extract and add values from rowA
    for (size_t i = 0; i < constructorA.getNumSelectors(); ++i) {
        Term value = tableA.accessField(rowA, constructorA[i].getName()); // Extract the i-th value from rowA
        combinedValues.push_back(value);
    }

    // Extract and add values from rowB
    for (size_t i = 0; i < constructorB.getNumSelectors(); ++i) {
        Term value = tableB.accessField(rowB, constructorB[i].getName()); // Extract the i-th value from rowB
        combinedValues.push_back(value);
    }

    // Create a new row for the joined table
    Term newRow = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, {combinedValues});

    return newRow;
}

Term combineRowsB(const Term &rowA, Table tableA, const Term &rowB, Table tableB, const Sort &joinedTableSort, Solver& slv) {
    // Assuming the first datatype constructor is used for each table row
    const DatatypeConstructor &constructorA = rowA.getSort().getDatatype()[0];
    const DatatypeConstructor &constructorB = rowB.getSort().getDatatype()[0];
    const DatatypeConstructor &joinedConstructor = joinedTableSort.getDatatype()[0];

    std::vector<Term> combinedValues;
    combinedValues.push_back(joinedConstructor.getTerm());

    // Extract and add values from rowA
    for (size_t i = 0; i < constructorA.getNumSelectors(); ++i) {
        Term value = tableA.accessField(rowA, constructorA[i].getName()); // Extract the i-th value from rowA
        combinedValues.push_back(value);
    }

    // Extract and add values from rowB
    for (size_t i = 0; i < constructorB.getNumSelectors(); ++i) {
        Term value = tableB.accessField(rowB, constructorB[i].getName()); // Extract the i-th value from rowB
        combinedValues.push_back(value);
    }

    // Create a new row for the joined table
    Term newRow = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, {combinedValues});

    return newRow;
}

Term convertBagToBagOfTuples(Solver &slv, Term bag) {
    Sort bagOfCustomSort = slv.mkBagSort(bag.getSort().getBagElementSort());
    const DatatypeConstructor &constructorA = bag.getSort().getBagElementSort().getDatatype()[0];
    vector<Sort> bagSorts;
    for (size_t i = 0; i < constructorA.getNumSelectors(); ++i) {
        bagSorts.push_back(constructorA[i].getCodomainSort());
    }

    Sort tupleSort = slv.mkTupleSort(bagSorts);
    Sort tableSort = slv.mkBagSort(tupleSort);

    // Define a lambda function for conversion
    Term customElementVar = slv.mkVar(bag.getSort().getBagElementSort(), "element");
    std::vector<Term> varsSetAList = {customElementVar};
    Term varListAList = slv.mkTerm(Kind::VARIABLE_LIST, varsSetAList);

    vector<Term> tupleFields;
    tupleFields.push_back(tupleSort.getDatatype()[0].getTerm());
    for (size_t i = 0; i < constructorA.getNumSelectors(); ++i) {
        const DatatypeSelector& selector = constructorA[i];
        Term field = slv.mkTerm(Kind::APPLY_SELECTOR, {selector.getTerm(), customElementVar});
        tupleFields.push_back(field);
    }

    // Construct the tuple
    Term tuple = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, {tupleFields});
    // Define the lambda function for conversion
    Term conversionLambda = slv.mkTerm(Kind::LAMBDA, {varListAList, tuple});

// Apply the lambda function to the bag
    Term convertedBag = slv.mkTerm(Kind::BAG_MAP, {conversionLambda, bag});

    return convertedBag;
}

Term convertBagOfTuplesToCustom(Solver &slv, Term bagOfTuples, Table table) {
    // Assuming tupleSort and customDatatypeSort are defined
    Sort tupleSort = table.getBagTableSort(); // Tuple sort
    Sort customDatatypeSort = bagOfTuples.getSort(); // Custom datatype sort

    Term constantTable = slv.mkConst(tupleSort, "backRow");

    // Define a variable for the tuple
    Term tupleVar = slv.mkVar(constantTable.getSort(), "tupleVar1");
    std::vector<Term> varsSetAList = {tupleVar};
    Term varListAList = slv.mkTerm(Kind::VARIABLE_LIST, varsSetAList);

    Term tupleBagVar = slv.mkVar(customDatatypeSort.getBagElementSort(), "tupleVar2");
    std::vector<Term> varsSetBList = {tupleBagVar};
    Term varListBList = slv.mkTerm(Kind::VARIABLE_LIST, varsSetBList);

    // Extract fields from the tuple
    vector<Term> extractedFields;
    const Datatype& tupleDatatype = tupleSort.getDatatype();
    const DatatypeConstructor& tupleConstructor = tupleDatatype[0];
    extractedFields.push_back(tupleConstructor.getTerm());
    for (size_t i = 0; i < tupleConstructor.getNumSelectors(); ++i) {
        const DatatypeSelector& selector = tupleConstructor[i];
        Term field = slv.mkTerm(Kind::APPLY_SELECTOR, {selector.getTerm(), tupleVar});
        extractedFields.push_back(field);
    }

    // Construct an element of the custom datatype
    // Assuming the custom datatype constructor is defined
    Term customElement = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, {extractedFields});
    // Define the lambda function for conversion
    Term conversionLambda = slv.mkTerm(Kind::LAMBDA, {varListBList, customElement});

    // Apply the lambda function to the bag of tuples
    Term convertedBag = slv.mkTerm(Kind::BAG_MAP, {conversionLambda, bagOfTuples});

    return convertedBag;
}


ProjectionReturn RelationalAlgebras::Product(Solver &slv, Table tableA, Table tableB, const Term &setA, const Term &setB) {
    // Convert Bags
//    Term bagA = convertBagToBagOfTuples(slv, setA);
//    Term bagB = convertBagToBagOfTuples(slv, setB);
//    Term product = slv.mkTerm(Kind::TABLE_PRODUCT, {bagA, bagB});
    Term product = slv.mkTerm(Kind::TABLE_PRODUCT, {setA, setB});
//    cout << product << " / " << product.getSort() << endl << endl << endl;
    // Prepare datatype for the joined table
    std::string joinedTableName = tableA.getTableName() + "_join_" + tableB.getTableName();
    DatatypeDecl joinedTableDatatype = slv.mkDatatypeDecl(joinedTableName);
    DatatypeConstructorDecl joinedTableConstructor = slv.mkDatatypeConstructorDecl("make-" + joinedTableName);
    // Define the new datatype by combining the datatypes from both tables
    // add columns from the first table
    // Table

    // Bag
//    const Datatype &firstTableDatatype = tableA.getTableSort().getDatatype();
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
//        joinedTableConstructor.addSelector(columnName, columnSort);
        //Table
        tableSorts.push_back(columnSort);
    }

    // add columns from the second table, excluding the join column
    for (size_t i = 0; i < secondTableConstructor.getNumSelectors(); ++i) {
        std::string columnName = secondTableConstructor[i].getName();
        Sort columnSort = secondTableConstructor[i].getCodomainSort();
//        joinedTableConstructor.addSelector(columnName, columnSort);
        //Table
        tableSorts.push_back(columnSort);
    }
//    joinedTableDatatype.addConstructor(joinedTableConstructor);
//    Sort joinedTableSort = slv.mkDatatypeSort(joinedTableDatatype);
    // Bag
//    Sort joinedSort = slv.mkSetSort(joinedTableSort);
//    Sort joinedSort = slv.mkBagSort(joinedTableSort);

    //Table
    Sort tupleSort = slv.mkTupleSort(tableSorts);
    Sort joinedSort = slv.mkBagSort(tupleSort);

    // Create joined tables
    std::vector<std::pair<std::string, cvc5::Sort>> columnNamesB = tableB.getColumnNames();
    std::vector<std::pair<std::string, cvc5::Sort>> concatenated = tableA.getColumnNames();
    // Insert all elements of the second vector at the end of the first
    concatenated.insert(concatenated.end(), columnNamesB.begin(), columnNamesB.end());

    Table joinedTable(slv, joinedTableName, joinedSort, concatenated);
    return ProjectionReturn{product, joinedTable};

    // Bag Conversion
//    Term convertedProduct = convertBagOfTuplesToCustom(slv, product, joinedTable);
    // Bag
//    Term boundVar = slv.mkVar(joinedTable.getTableSort(), "row");
//    Term boundVar = slv.mkVar(joinedTable.getBagTableSort(), "row");
//
//    std::vector<Term> vars = {boundVar};
//    Term varList = slv.mkTerm(Kind::VARIABLE_LIST, vars);
//
////    Term boundVarA = slv.mkVar(tableA.getTableSort(), "rowA");
//    Term boundVarA = slv.mkVar(tableA.getBagTableSort(), "rowA");
//    std::vector<Term> varsA = {boundVarA};
//    Term varListA = slv.mkTerm(Kind::VARIABLE_LIST, varsA);
//
////    Term boundVarB = slv.mkVar(tableB.getTableSort(), "rowB");
//    Term boundVarB = slv.mkVar(tableB.getBagTableSort(), "rowB");
//    std::vector<Term> varsB = {boundVarB};
//    Term varListB = slv.mkTerm(Kind::VARIABLE_LIST, varsB);
//
//    std::vector<Term> varsC = {boundVarA, boundVarB};
//    Term varListC = slv.mkTerm(Kind::VARIABLE_LIST, varsC);
//
//    Term boundVarSetA = slv.mkVar(setA.getSort(), "rowSetA");
//    std::vector<Term> varsSetAList = {boundVarSetA};
//    Term varListAList = slv.mkTerm(Kind::VARIABLE_LIST, varsSetAList);
//
//    Term boundVarSetB = slv.mkVar(setB.getSort(), "rowSetB");
//    std::vector<Term> varsSetBList = {boundVarSetB};
//    Term varListBList = slv.mkTerm(Kind::VARIABLE_LIST, varsSetBList);
//
//    std::vector<Term> varsIn = {boundVarSetA, boundVarSetB};
//    Term varListIn = slv.mkTerm(Kind::VARIABLE_LIST, varsIn);
//
//
//    // Define the lambda function for transformation
//    Term lambda = slv.mkTerm(Kind::LAMBDA, {varListIn, combineRows(boundVarA, tableA, boundVarB, tableB, joinedTableSort, slv)});
//    cout << lambda << endl;
//
////    Term product = slv.mkTerm(Kind::TABLE_PRODUCT, {setA, setB});
//
//    std::vector<Sort> inputSorts = {tableA.getTableSort(), tableB.getTableSort()};
//    Sort crossProductFunctionSort = slv.mkFunctionSort(inputSorts, slv.mkSetSort(joinedTable.getTableSort()));
//    // Declare a function symbol with the cross product function sort
//    Term crossProductFunction = slv.mkConst(crossProductFunctionSort, "crossProduct");
//
//        //Flat map over map nested
//        // Alloy and Coq
//        // Alloy analyzer for smt solver
//    // Use SET_MAP to map over setA, and for each element, map over setB
//    Term lambda2 = slv.mkTerm(Kind::LAMBDA, {varListBList, combineRowsB(boundVarA, tableA, boundVarB, tableB, joinedTableSort, slv)});
//    cout << lambda2 << endl;
////    Term unionSet = slv.mkTerm(Kind::SET_MAP, {lambda1, slv.mkTerm(Kind::SET_MAP, {lambda2, setB})});
////    Term setBMap = slv.mkTerm(Kind::SET_MAP, {lambda, setB});
////    cout << setBMap << endl;
//
//
//    // Define a lambda function that takes an element from setA and returns its cross product with setB
//    Term crossProductResult = slv.mkTerm(Kind::APPLY_UF, {lambda, setA, setB});
//    cout << crossProductResult << " " << crossProductResult.getSort() << " " << crossProductResult.getSort().getDatatype() << endl;
//    cout << crossProductResult.getSort().getDatatype() << endl;
//    return ProjectionReturn{crossProductResult, joinedTable};
}

