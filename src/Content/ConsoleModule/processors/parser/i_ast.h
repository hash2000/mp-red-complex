#pragma once
#include "Content/ConsoleModule/processors/parser/source_cursor.h"

using IAstPtr = std::shared_ptr<class IAst>;
using IAstCollection = std::vector<IAstPtr>;

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

	virtual void setChild(IAstPtr child) = 0;

	virtual IAstCollection& children() const = 0;
};
