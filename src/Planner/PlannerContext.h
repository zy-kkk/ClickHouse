#pragma once

#include <Common/HashTable/Hash.h>

#include <Core/Names.h>
#include <Core/NamesAndTypes.h>

#include <Interpreters/Context_fwd.h>
#include <Interpreters/SubqueryForSet.h>

#include <Analyzer/IQueryTreeNode.h>

namespace DB
{

namespace ErrorCodes
{
    extern const int LOGICAL_ERROR;
}

using ColumnIdentifier = std::string;

class TableExpressionColumns
{
public:
    using ColumnNameToColumnIdentifier = std::unordered_map<std::string, ColumnIdentifier>;

    bool hasColumn(const std::string & column_name) const
    {
        return alias_columns_names.contains(column_name) || columns_names.contains(column_name);
    }

    void addColumn(const NameAndTypePair & column, const ColumnIdentifier & column_identifier)
    {
        if (hasColumn(column.name))
            throw Exception(ErrorCodes::LOGICAL_ERROR, "Column with name {} already exists");

        columns_names.insert(column.name);
        columns.push_back(column);
        column_name_to_column_identifier.emplace(column.name, column_identifier);
    }

    void addColumnIfNotExists(const NameAndTypePair & column, const ColumnIdentifier & column_identifier)
    {
        if (hasColumn(column.name))
            return;

        columns_names.insert(column.name);
        columns.push_back(column);
        column_name_to_column_identifier.emplace(column.name, column_identifier);
    }

    void addAliasColumnName(const std::string & column_name)
    {
        alias_columns_names.insert(column_name);
    }

    const NameSet & getAliasColumnsNames() const
    {
        return alias_columns_names;
    }

    const NameSet & getColumnsNames() const
    {
        return columns_names;
    }

    const NamesAndTypesList & getColumns() const
    {
        return columns;
    }

    const ColumnNameToColumnIdentifier & getColumnNameToIdentifier() const
    {
        return column_name_to_column_identifier;
    }

    const ColumnIdentifier & getColumnIdentifierOrThrow(const std::string & column_name) const
    {
        auto it = column_name_to_column_identifier.find(column_name);
        if (it == column_name_to_column_identifier.end())
            throw Exception(ErrorCodes::LOGICAL_ERROR,
                "Column identifier for name {} does not exists",
                column_name);

        return it->second;
    }

    const ColumnIdentifier * getColumnIdentifierOrNull(const std::string & column_name) const
    {
        auto it = column_name_to_column_identifier.find(column_name);
        if (it == column_name_to_column_identifier.end())
            return nullptr;

        return &it->second;
    }

private:
    /// Valid for table, table function, query table expression nodes
    NamesAndTypesList columns;

    /// Valid for table, table function, query table expression nodes
    NameSet columns_names;

    /// Valid only for table table expression node
    NameSet alias_columns_names;

    /// Valid for table, table function, query table expression nodes
    ColumnNameToColumnIdentifier column_name_to_column_identifier;
};

class Set;
using SetPtr = std::shared_ptr<Set>;

class GlobalPlannerContext
{
public:
    GlobalPlannerContext() = default;

    void registerSet(UInt128 source_hash, SetPtr set)
    {
        set_source_to_set.emplace(source_hash, set);
    }

    SetPtr getSet(UInt128 source_hash) const
    {
        auto it = set_source_to_set.find(source_hash);
        if (it == set_source_to_set.end())
            return nullptr;

        return it->second;
    }

    void registerSubqueryForSet(String key, SubqueryForSet subquery_for_set)
    {
        subqueries_for_sets.emplace(key, std::move(subquery_for_set));
    }

    const SubqueriesForSets & getSubqueriesForSets() const
    {
        return subqueries_for_sets;
    }
private:
    std::unordered_map<UInt128, SetPtr, UInt128Hash> set_source_to_set;

    SubqueriesForSets subqueries_for_sets;
};

using GlobalPlannerContextPtr = std::shared_ptr<GlobalPlannerContext>;

class PlannerContext
{
public:
    PlannerContext(ContextPtr query_context_, GlobalPlannerContextPtr global_planner_context_);

    const ContextPtr & getQueryContext() const
    {
        return query_context;
    }

    const GlobalPlannerContextPtr & getGlobalPlannerContext() const
    {
        return global_planner_context;
    }

    GlobalPlannerContextPtr & getGlobalPlannerContext()
    {
        return global_planner_context;
    }

    const std::unordered_map<const IQueryTreeNode *, std::string> & getTableExpressionNodeToIdentifier() const
    {
        return table_expression_node_to_identifier;
    }

    std::unordered_map<const IQueryTreeNode *, std::string> & getTableExpressionNodeToIdentifier()
    {
        return table_expression_node_to_identifier;
    }

    const std::unordered_map<const IQueryTreeNode *, TableExpressionColumns> & getTableExpressionNodeToColumns() const
    {
        return table_expression_node_to_columns;
    }

    std::unordered_map<const IQueryTreeNode *, TableExpressionColumns> & getTableExpressionNodeToColumns()
    {
        return table_expression_node_to_columns;
    }

    ColumnIdentifier getColumnUniqueIdentifier(const IQueryTreeNode * column_source_node, std::string column_name = {});

    void registerColumnNode(const IQueryTreeNode * column_node, const ColumnIdentifier & column_identifier);

    const ColumnIdentifier & getColumnNodeIdentifierOrThrow(const IQueryTreeNode * column_node);

    const ColumnIdentifier * getColumnNodeIdentifierOrNull(const IQueryTreeNode * column_node);

private:
    /// Query context
    ContextPtr query_context;

    /// Global planner context
    GlobalPlannerContextPtr global_planner_context;

    /// Column node to column identifier
    std::unordered_map<const IQueryTreeNode *, ColumnIdentifier> column_node_to_column_identifier;

    /// Table expression to identifier
    std::unordered_map<const IQueryTreeNode *, std::string> table_expression_node_to_identifier;

    /// Table expression node to columns
    std::unordered_map<const IQueryTreeNode *, TableExpressionColumns> table_expression_node_to_columns;

    size_t column_identifier_counter = 0;
};

}
