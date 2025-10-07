#ifndef TPC_CPP_VISITORNODES_H
#define TPC_CPP_VISITORNODES_H

#include <utility>

#include "Visitor.h"

class Context {
    Term resultOperation;
    Table resultTable;

    // Use a shared pointer to share the state
    map<string, pair<Table, Term>> myState;

public:
    Context(Solver& slv, map<string, pair<Table, Term>> initialState)
            : resultTable(Table(slv)), myState(std::move(initialState)) {}

    void setResultOperation(const Term& res) { resultOperation = res; }
    Term getResultOperation() const { return resultOperation; }

    void setState(const string& tableName, const pair<Table, Term>& newUpdate) { myState[tableName] = newUpdate; }
    map<string, pair<Table, Term>> getState() const { return myState; }

    void setResultTable(const Table& newTable) { resultTable = newTable; }
    Table getResultTable() const { return resultTable; }

};

class Node {
public:
    virtual void accept(Visitor &v, Context &context) = 0;
};

class TableNode : public Node {
private:
    Solver& slv;
    Table table;
    Term tableTerm;
    string tableName;

public:
    TableNode(Solver& slv, const Table& inputTable, const Term& inputTerm, string tableName) : table(inputTable), tableTerm(inputTerm), slv(slv), tableName(std::move(tableName)) {}

    Table getTable() const { return table; }
    Term getTableTerm() const { return tableTerm; }
    Solver& getSolver() { return slv; }
    string getTableName() const { return tableName; }

    void accept(Visitor &v, Context &context) override {
        // cout << "Table Node Accepted" << endl;
        v.visit(*this, context);
    }
};
class DeleteNode : public Node {
private:
    Node* child;
    Solver& slv;
    RowData row;
    string tableName;

public:
    DeleteNode(Node* childNode, Solver& slv, RowData newRow, string tableName)
            : slv(slv), child(childNode), row(std::move(newRow)), tableName(std::move(tableName)) { }

    Solver& getSolver() { return slv; }
    string getTableName() const { return tableName; }
    RowData getDeleteRow() const { return row; }

    void accept(Visitor &v, Context &context) override {
        // cout << "Insert Node Accepted" << endl;
        // First visit the child
        child->accept(v, context);
        // Apply select
        v.visit(*this, context);
    }
};

class InsertNode : public Node {
private:
    Node* child;
    Solver& slv;
    RowData row;
    string tableName;

public:
    InsertNode(Node* childNode, Solver& slv, RowData newRow, string tableName)
            : slv(slv), child(childNode), row(newRow), tableName(std::move(tableName)) {}

    Solver& getSolver() { return slv; }
    string getTableName() const { return tableName; }
    RowData getNewRow() const { return row; }

    void accept(Visitor &v, Context &context) override {
        // cout << "Insert Node Accepted" << endl;
        // First visit the child
        child->accept(v, context);
        // Apply select
        v.visit(*this, context);
    }
};

class UpdateNode : public Node {
private:
    Node* child;
    Solver& slv;
    string columnName;
    variant<string, int> value;
    string selectOperation;
    string tableName;
    pair<string, variant<std::string, int>> updateValue;

public:
    UpdateNode(Node* childNode, Solver& slv, string colName,
    const variant<string, int> &val, string selOp, string tableName, const pair<string, variant<std::string, int>> &updateVal)
    : slv(slv), child(childNode), columnName(std::move(colName)), value(val), selectOperation(std::move(selOp)),
    tableName(std::move(tableName)), updateValue(updateVal) { }

    string getColumnName() const { return columnName; }
    variant<string, int> getValue() const { return value; }
    string getSelectOperation() const { return selectOperation; }
    Solver& getSolver() { return slv; }
    string getTableName() const { return tableName; }
    pair<string, variant<std::string, int>> getNewValue() const { return updateValue; }

    void accept(Visitor &v, Context &context) override {
        // cout << "Update Node Accepted" << endl;
        // First visit the child
        child->accept(v, context);
        // Apply select
        v.visit(*this, context);
    }
};

class SelectNode : public Node {
private:
    Node* child;
    Solver& slv;
    string columnName;
    variant<string, int> value;
    string selectOperation;
    string tableName;

public:
    SelectNode(Node* childNode, Solver& slv, string colName,
               const variant<string, int> &val, string selOp, string tableName)
            : slv(slv), child(childNode), columnName(std::move(colName)), value(val), selectOperation(std::move(selOp)), tableName(std::move(tableName)) { }

    string getColumnName() const { return columnName; }
    variant<string, int> getValue() const { return value; }
    string getSelectOperation() const { return selectOperation; }
    Solver& getSolver() { return slv; }
    string getTableName() const { return tableName; }

    void accept(Visitor &v, Context &context) override {
        // cout << "Select Node Accepted" << endl;
        // First visit the child
        child->accept(v, context);
        // Apply select
        v.visit(*this, context);
    }
};

class ProjectNode : public Node {
private:
    Node* child;
    Solver& slv;
    vector<string> desiredColumns;
    string tableName;

public:
    ProjectNode(Node* childNode, Solver& slv, vector<string> columns, string tableName) : slv(slv), desiredColumns(std::move(columns)), child(childNode), tableName(std::move(tableName)) {}

    const vector<string>& getDesiredColumns() const { return desiredColumns; }
    Solver& getSolver() { return slv; }
    string getTableName() const { return tableName; }

    void accept(Visitor &v, Context &context) override {
        // cout << "Project Node Accepted" << endl;
        // First visit the child
        child->accept(v, context);

        // Apply select
        v.visit(*this, context);
    }
};

class ProductNode : public Node {
private:
    Node* leftChild;
    Node* rightChild;
    Solver& slv;
    string tableNameLeft;
    string tableNameRight;
    string joinColumnLeft;
    string joinColumnRight;

public:
    ProductNode(Node* left, Node* right,  Solver& slv, string newTableNameLeft, string newTableNameRight, string joinColumnLeft,
                string joinColumnRight) : leftChild(left), rightChild(right), slv(slv),
                tableNameLeft(std::move(newTableNameLeft)), tableNameRight(newTableNameRight),
                joinColumnLeft(joinColumnLeft), joinColumnRight(joinColumnRight) {}

    Solver& getSolver() { return slv; }
    string getTableNameLeft() const { return tableNameLeft; }
    string getTableNameRight() const { return tableNameRight; }
    string getJoinColumnRight() const { return joinColumnRight; }
    string getJoinColumnLeft() const { return joinColumnLeft; }

    void accept(Visitor &v, Context &context) override {
        // cout << "Product Node Accepted" << endl;
        // Create separate contexts for left and right children
        Context leftContext(slv, context.getState()), rightContext(slv, context.getState());

        // Process left child
        if (leftChild) {
            leftChild->accept(v, leftContext);
        }
        // Process right child
        if (rightChild) {
            rightChild->accept(v, rightContext);
        }
        // Combine the results from left and right in the current context
        v.visit(*this, leftContext, rightContext, context);
    }
};

class AggregateNode : public Node {
private:
    Node* child;
    Solver& slv;
    uint32_t columnIndex;

public:
    AggregateNode(Node* childNode, Solver& slv, uint32_t columnIndex) : slv(slv), child(childNode), columnIndex(columnIndex) { }

    uint32_t getColumnIndex() const { return columnIndex; }
    Solver& getSolver() { return slv; }

    void accept(Visitor &v, Context &context) override {
        // cout << "Aggregate Node Accepted" << endl;
        // First visit the child
        child->accept(v, context);
        // Apply select
        v.visit(*this, context);
    }
};


class ConcreteVisitor : public Visitor {
public:
    void visit(TableNode &node, Context &context) override {
        // Initialize Context and move up
        // Update context or handle the result as needed
        context.setResultOperation(node.getTableTerm());
        context.setResultTable(node.getTable());

        context.setState(node.getTableName(), make_pair(node.getTable(), node.getTableTerm()));
    }

    void visit(InsertNode &node, Context &context) override {
        // Initialize Context and move up
        // Update context or handle the result as needed
        Term result = RelationalAlgebras::Insert(node.getSolver(), context.getResultTable(), context.getResultOperation(), node.getNewRow());

        context.setResultOperation(result);
        context.setResultTable(context.getResultTable());

        context.setState(node.getTableName(), make_pair(context.getResultTable(), result));
    }

    void visit(DeleteNode &node, Context &context) override {
        // Initialize Context and move up
        // Update context or handle the result as needed
        Term result = RelationalAlgebras::Delete(node.getSolver(), context.getResultTable(), context.getResultOperation(), node.getDeleteRow());

        context.setResultOperation(result);
        context.setResultTable(context.getResultTable());

        context.setState(node.getTableName(), make_pair(context.getResultTable(), result));
    }

    void visit(SelectNode &node, Context &context) override {
        // Node's attributes to perform the select operation
        Term result = RelationalAlgebras::Select(node.getSolver(), context.getResultTable(), context.getResultOperation(),
                                                 node.getColumnName(), node.getValue(), node.getSelectOperation());
        // Update context or handle the result
        context.setResultOperation(result);
        context.setResultTable(context.getResultTable());

        context.setState(node.getTableName(), make_pair(context.getResultTable(), result));
    }

    void visit(UpdateNode &node, Context &context) override {
        // Node's attributes to perform the select operation
        Term result = RelationalAlgebras::Update(node.getSolver(), context.getResultTable(), context.getResultOperation(),
                                                 node.getColumnName(), node.getValue(), node.getSelectOperation(), node.getNewValue());
        // Update context or handle the result
        context.setResultOperation(result);
        context.setResultTable(context.getResultTable());

        context.setState(node.getTableName(), make_pair(context.getResultTable(), result));
    }

    void visit(ProjectNode &node, Context &context) override {
        // Node's attributes to perform the Project operation
        ProjectionReturn result = RelationalAlgebras::Project(
                node.getSolver(),
                context.getResultTable(),
                context.getResultOperation(),
                node.getDesiredColumns()
        );

        // Update the context with the result of the projection
        context.setResultOperation(result.setComprehension);
        context.setResultTable(result.newTable);

        context.setState(node.getTableName(), make_pair(result.newTable, result.setComprehension));
    }

    void visit(ProductNode &node, Context &leftContext, Context &rightContext, Context &context) override {
        // Get the tables and terms from the left and right contexts
        Table leftTable = leftContext.getResultTable();
        Term leftTerm = leftContext.getResultOperation();

        Table rightTable = rightContext.getResultTable();
        Term rightTerm = rightContext.getResultOperation();

        // Call the Product function with the tables and terms from both sides
        ProjectionReturn result = RelationalAlgebras::Product(node.getSolver(), leftTable, rightTable, leftTerm, rightTerm, node.getJoinColumnLeft(), node.getJoinColumnRight());

        // Update the current context with the result
        context.setResultOperation(result.setComprehension);
        context.setResultTable(result.newTable);

        context.setState(node.getTableNameLeft(), make_pair(leftTable, leftTerm));
        context.setState(node.getTableNameRight(), make_pair(rightTable, rightTerm));

        string joinedTables = node.getTableNameLeft() + "_join_" + node.getTableNameRight();
        context.setState(joinedTables, make_pair(result.newTable, result.setComprehension));
    }

    void visit(AggregateNode &node, Context &context) override {
        // Node's attributes to perform the Project operation
        Term result = RelationalAlgebras::Aggregation(
                node.getSolver(),
                context.getResultTable(),
                context.getResultOperation(),
                1
        );

        // Update the context with the result of the projection
        context.setResultOperation(result);

//        context.setState(node.getTableName(), make_pair(context.getResultTable(), result));
    }
};


#endif //TPC_CPP_VISITORNODES_H
