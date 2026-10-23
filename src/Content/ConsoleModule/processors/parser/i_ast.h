#pragma once
#include "Content/ConsoleModule/processors/parser/source_cursor.h"

#include <memory>
#include <map>
#include <vector>

using IAstPtr = std::shared_ptr<class IAst>;
using IAstCollection = std::vector<IAstPtr>;
using IAstGroupegCollection = std::map<QString, IAstCollection>;

class IAst {
public:
	IAst() = default;
	IAst(const IAst&) = default;
	virtual ~IAst() = default;
	IAst& operator=(const IAst&) = default;

	virtual void setLocation(const SourceLocation& loc) = 0;

	virtual SourceLocation location() const = 0;

	virtual void setParameter(const QString& name, const QVariant& value) = 0;

	virtual QVariant parameter(const QString& name) const = 0;

	virtual void setChild(const QString& type, IAstPtr child) = 0;

	virtual void setChildren(const QString& type, const IAstCollection& children) = 0;

	virtual size_t childTypesCount() const = 0;

	virtual std::list<QString> childTypes() const = 0;

	virtual IAstCollection children(const QString& type) const = 0;
};
