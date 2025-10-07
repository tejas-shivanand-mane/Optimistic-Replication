
#ifndef TPC_CPP_VISITOR_H
#define TPC_CPP_VISITOR_H

#include <iostream>
#include <cvc5/cvc5.h>
#include "Table.h"
#include "TableInfo.h"
#include "RelationalAlgebras.h"
#include "RelationalProver.h"
#include <utility>

using namespace std;
using namespace cvc5;

class InsertNode;
class DeleteNode;
class TableNode;
class ProjectNode;
class UpdateNode;
class SelectNode;
class ProductNode;
class AggregateNode;
class Context;


class Visitor {
public:
    virtual void visit(InsertNode &node, Context &context) = 0;
    virtual void visit(DeleteNode &node, Context &context) = 0;
    virtual void visit(TableNode &node, Context &context) = 0;
    virtual void visit(SelectNode &node, Context &context) = 0;
    virtual void visit(UpdateNode &node, Context &context) = 0;
    virtual void visit(ProjectNode &node, Context &context) = 0;
    virtual void visit(ProductNode &node, Context &leftContext, Context &rightContext, Context &context) = 0;
    virtual void visit(AggregateNode &node, Context &context) = 0;
};




#endif //TPC_CPP_VISITOR_H
