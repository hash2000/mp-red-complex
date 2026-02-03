#pragma once
#include <QVariantMap>

class VariablesContext {
public:
	QVariant get(const QString& name, const QString& def);
	QVariant set(const QString& name, const QString& value);

	QStringList available() const;

private:
	QVariantMap _variables;
};
