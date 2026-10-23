#pragma once
#include <QVariantMap>

class VariablesContext {
public:
	QVariant get(const QString& name, const QVariant& def) const;
	QVariant set(const QString& name, const QVariant& value);
	void clear();
	void apply(const VariablesContext& values);

	QStringList available() const;

private:
	QVariantMap _variables;
};
