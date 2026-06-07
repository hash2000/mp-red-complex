#pragma once
#include <QVariantMap>

class VariablesContext {
public:
	QVariant get(const QString& name, const QVariant& def);
	QVariant set(const QString& name, const QVariant& value);

	QStringList available() const;

private:
	QVariantMap _variables;
};
