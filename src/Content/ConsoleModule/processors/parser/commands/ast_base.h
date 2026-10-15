#pragma once
#include "Content/ConsoleModule/processors/parser/i_ast.h"
#include <QVariantMap>
#include <memory>
#include <vector>

class AstBase : public IAst {
public:
	AstBase();
	~AstBase() override;

	void setLocation(const SourceLocation& loc) override;

	SourceLocation location() const override;

	void setParameter(const QString& name, const QVariant& value) override;

	QVariant parameter(const QString& name) const override;

	void setChild(IAstPtr child) override;

	IAstCollection& children() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
