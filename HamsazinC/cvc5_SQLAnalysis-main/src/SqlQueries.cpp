#include "../header/SqlQueries.h"


RowData convertToRowData(const std::vector<std::string>& insertData) {
    RowData rowData;

    for (const auto& str : insertData) {
        try {
            // Try to convert to int64_t
            int64_t intVal = std::stoll(str);
            rowData.emplace_back(intVal);
        } catch (const std::invalid_argument&) {
            try {
                // Not an int. Try to convert to double
                double doubleVal = std::stod(str);
                rowData.emplace_back(doubleVal);
            } catch (const std::invalid_argument&) {
                // Not a double either. Treat as string
                rowData.emplace_back(str);
            }
        }
    }

    return rowData;
}


//// Selection on Projection: SelectPredicate(ProjectColumns(Ta))
Context SqlQueries::Query1(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs) {
    const string& tableAName = tableNames[0];

    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;

    // Create a visitor and context
    ConcreteVisitor visitor;
    Context context(slv, initialState);

    std::vector<std::string> projectColumns;
    std::vector<std::string> selectConditions;
    if (kwargs.find("projectColumns") != kwargs.end()) {
        projectColumns = vector<string>(kwargs.at("projectColumns"));
    } else {throw std::runtime_error("projectColumns not found");}
    if (kwargs.find("selectPredicate") != kwargs.end()) {
        selectConditions = vector<string>(kwargs.at("selectPredicate"));
    } else {throw std::runtime_error("selectPredicate not found");}


    // Create table nodes
    auto* addressTableNode1 = new TableNode(slv, tableA, tableATerm, tableAName);
    auto* projectNode1 = new ProjectNode(addressTableNode1, slv, projectColumns, tableAName);
    auto* selectNode1 = new SelectNode(projectNode1, slv, selectConditions[0], selectConditions[2], selectConditions[1], tableAName);
    selectNode1->accept(visitor, context);

    return context;
}

//// Projection on Selection: ProjectColumns(SelectPredicate(Ta))
Context SqlQueries::Query2(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs) {
    const string& tableAName = tableNames[0];
    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;

    // Create a visitor and context
    ConcreteVisitor visitor;
    Context context(slv, initialState);

    std::vector<std::string> projectColumns;
    std::vector<std::string> selectConditions;
    if (kwargs.find("projectColumns") != kwargs.end()) {
        projectColumns = vector<string>(kwargs.at("projectColumns"));
    } else {throw std::runtime_error("projectColumns not found");}
    if (kwargs.find("selectPredicate") != kwargs.end()) {
        selectConditions = vector<string>(kwargs.at("selectPredicate"));
    } else {throw std::runtime_error("selectPredicate not found");}

    auto* addressTableNode2 = new TableNode(slv, tableA, tableATerm, tableAName);
    auto* selectNode2 = new SelectNode(addressTableNode2, slv, selectConditions[0], selectConditions[2], selectConditions[1], tableAName);
    auto* projectNode2 = new ProjectNode(selectNode2, slv, projectColumns, tableAName);
    projectNode2->accept(visitor, context);
    return context;
}

////// Insert then Update: SelectPredicate(ProjectColumns(Ta))
//Context SqlQueries::Query3(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs) {
//    const string& tableAName = tableNames[0];
//
//    Table tableA = initialState[tableAName].first;
//    Term tableATerm = initialState[tableAName].second;
//
//    // Create a visitor and context
//    ConcreteVisitor visitor;
//    Context context(slv, initialState);
//
//    std::vector<std::string> projectColumns;
//    std::vector<std::string> selectConditions;
//    if (kwargs.find("projectColumns") != kwargs.end()) {
//        projectColumns = vector<string>(kwargs.at("projectColumns"));
//    } else {throw std::runtime_error("projectColumns not found");}
//    if (kwargs.find("selectPredicate") != kwargs.end()) {
//        selectConditions = vector<string>(kwargs.at("selectPredicate"));
//    } else {throw std::runtime_error("selectPredicate not found");}
//
//
//    // Create table nodes
//    auto* addressTableNode1 = new TableNode(slv, tableA, tableATerm, tableAName);
//    auto* projectNode1 = new ProjectNode(addressTableNode1, slv, projectColumns, tableAName);
//    auto* selectNode1 = new SelectNode(projectNode1, slv, selectConditions[0], selectConditions[2], selectConditions[1], tableAName);
//    selectNode1->accept(visitor, context);
//
//    return context;
//}

//// Projection: ProjectColumns(Ta)
Context SqlQueries::Project(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs) {
    const string& tableAName = tableNames[0];

    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;

    // Create a visitor and context
    ConcreteVisitor visitor;
    Context context(slv, initialState);

    std::vector<std::string> projectColumns;
    if (kwargs.find("projectColumns") != kwargs.end()) {
        projectColumns = vector<string>(kwargs.at("projectColumns"));
    } else {throw std::runtime_error("projectColumns not found");}

    auto* TableANode = new TableNode(slv, tableA, tableATerm, tableAName);
    auto* projectNode1 = new ProjectNode(TableANode, slv, projectColumns, tableAName);
    projectNode1->accept(visitor, context);

    return context;
}

//// Selection: selectPredicate(Ta)
Context SqlQueries::Select(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs) {
    const string& tableAName = tableNames[0];

    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;

    // Create a visitor and context
    ConcreteVisitor visitor;
    Context context(slv, initialState);

    std::vector<std::string> selectConditions;
    if (kwargs.find("selectPredicate") != kwargs.end()) {
        selectConditions = vector<string>(kwargs.at("selectPredicate"));
    } else {throw std::runtime_error("selectPredicate not found");}

    auto* TableANode = new TableNode(slv, tableA, tableATerm, tableAName);
    auto* selectNode2 = new SelectNode(TableANode, slv, selectConditions[0], selectConditions[2], selectConditions[1], tableAName);
    selectNode2->accept(visitor, context);
    return context;
}

//// Product: Product(Ta, Tb)
Context SqlQueries::Product(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs) {
    const string& tableAName = tableNames[0];
    const string& tableBName = tableNames[1];

    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;

    Table tableB = initialState[tableBName].first;
    Term tableBTerm = initialState[tableBName].second;

    // Create a visitor and context
    ConcreteVisitor visitor;
    Context context(slv, initialState);

    vector<string> joinColumns;
    if (kwargs.find("joinColumns") != kwargs.end()) {
        joinColumns = vector<string>(kwargs.at("joinColumns"));
    } else {throw std::runtime_error("joinColumns not found");}

    auto* TableANode = new TableNode(slv, tableA, tableATerm, tableAName);
    auto* TableBNode = new TableNode(slv, tableB, tableBTerm, tableBName);
    auto* productNode = new ProductNode(TableANode, TableBNode, slv, tableAName, tableBName, joinColumns[0], joinColumns[1]);
    productNode->accept(visitor, context);
    return context;
}

//// Insert: InsertRow(Row -> Ta)
Context SqlQueries::Insert(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs) {
    const string& tableAName = tableNames[0];

    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;

    // Create a visitor and context
    ConcreteVisitor visitor;
    Context context(slv, initialState);

    vector<string> insertData;
    if (kwargs.find("insertRow") != kwargs.end()) {
        insertData = vector<string>(kwargs.at("insertRow"));
    } else {throw std::runtime_error("insertRow not found");}
    RowData newRow = convertToRowData(insertData);

    auto* TableANode = new TableNode(slv, tableA, tableATerm, tableAName);
    auto* insertNode = new InsertNode(TableANode, slv, newRow, tableAName);
    insertNode->accept(visitor, context);
    return context;
}

//// Delete: DeleteRow(Row -> Ta)
Context SqlQueries::Delete(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs) {
    const string& tableAName = tableNames[0];

    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;

    // Create a visitor and context
    ConcreteVisitor visitor;
    Context context(slv, initialState);

    vector<string> DeleteData;
    if (kwargs.find("deleteRow") != kwargs.end()) {
        DeleteData = vector<string>(kwargs.at("deleteRow"));
    } else {throw std::runtime_error("deleteRow not found");}
    RowData deleteRow = convertToRowData(DeleteData);

    auto* TableANode = new TableNode(slv, tableA, tableATerm, tableAName);
    auto* deleteNode = new DeleteNode(TableANode, slv, deleteRow, tableAName);
    deleteNode->accept(visitor, context);
    return context;
}

//// Update: Update(updateRow, Project(Select(Ta)))
Context SqlQueries::Update(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs){
const string& tableAName = tableNames[0];

    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;

    // Create a visitor and context
    ConcreteVisitor visitor;
    Context context(slv, initialState);

    vector<string> selectConditions;
    if (kwargs.find("selectPredicate") != kwargs.end()) {
        selectConditions = vector<string>(kwargs.at("selectPredicate"));
    } else {throw std::runtime_error("selectPredicate not found");}

    variant<string, int> value;
    try {
        int intValue = std::stoi(selectConditions[2]);
        value = intValue; // Set as int
    } catch (const std::invalid_argument&) {
        value = selectConditions[2]; // Set as string
    }

    vector<string> updateData;
    pair<string, variant<string, int>> updateValue;
    if (kwargs.find("updateRow") != kwargs.end()) {
        updateData = vector<string>(kwargs.at("updateRow"));
    } else {throw std::runtime_error("updateRow not found");}

    // Column to Update
    updateValue.first = updateData[0];
    try {
        int intValue = std::stoi(updateData[1]);
        updateValue.second = intValue; // Set as int
    } catch (const std::invalid_argument&) {
        updateValue.second = updateData[1]; // Set as string
    }


    auto* TableANode = new TableNode(slv, tableA, tableATerm, tableAName);
    auto* updateNode = new UpdateNode(TableANode, slv, selectConditions[0], value, selectConditions[1], tableAName, updateValue);
    updateNode->accept(visitor, context);
    return context;
}

//// Update: Update(updateRow, Project(Select(Ta)))
Context SqlQueries::JoinAndUpdate(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs){
    const string& tableAName = tableNames[0];
    const string& tableBName = tableNames[1];

    Table tableA = initialState[tableAName].first;
    Term tableATerm = initialState[tableAName].second;


    Table tableB = initialState[tableBName].first;
    Term tableBTerm = initialState[tableBName].second;

    // Create a visitor and context
    ConcreteVisitor visitor;
    Context context(slv, initialState);

    vector<string> selectConditions;
    if (kwargs.find("selectPredicate") != kwargs.end()) {
        selectConditions = vector<string>(kwargs.at("selectPredicate"));
    } else {throw std::runtime_error("selectPredicate not found");}

    variant<string, int> value;
    try {
        int intValue = std::stoi(selectConditions[2]);
        value = intValue; // Set as int
    } catch (const std::invalid_argument&) {
        value = selectConditions[2]; // Set as string
    }

    vector<string> updateData;
    pair<string, variant<string, int>> updateValue;
    if (kwargs.find("updateRow") != kwargs.end()) {
        updateData = vector<string>(kwargs.at("updateRow"));
    } else {throw std::runtime_error("updateRow not found");}

    // Column to Update
    updateValue.first = updateData[0];
    try {
        int intValue = std::stoi(updateData[1]);
        updateValue.second = intValue; // Set as int
    } catch (const std::invalid_argument&) {
        updateValue.second = updateData[1]; // Set as string
    }


    vector<string> joinColumns;
    if (kwargs.find("joinColumns") != kwargs.end()) {
        joinColumns = vector<string>(kwargs.at("joinColumns"));
    } else {throw std::runtime_error("joinColumns not found");}


    auto* TableANode = new TableNode(slv, tableA, tableATerm, tableAName);
    auto* TableBNode = new TableNode(slv, tableB, tableBTerm, tableBName);

    auto* productNode = new ProductNode(TableANode, TableBNode, slv, tableAName, tableBName, joinColumns[0], joinColumns[1]);

    auto* updateNode = new UpdateNode(productNode, slv, selectConditions[0], value, selectConditions[1], tableAName + "_join_" + tableBName, updateValue);
    updateNode->accept(visitor, context);
    return context;
}

