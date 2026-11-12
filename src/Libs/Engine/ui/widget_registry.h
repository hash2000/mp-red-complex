#pragma once
#include <QString>
#include <QWidget>
#include <memory>
#include <functional>

class CommandContext;

struct WidgetTypeHandler {
	QString id;
	QString title;
	QString glyph;
	QWidget* parent;
	std::shared_ptr<CommandContext> context;
};

class WidgetRegistry {
public:
	WidgetRegistry();
	virtual ~WidgetRegistry();

	using WidgetFactory = std::function<QWidget*(const QString& type, const WidgetTypeHandler& handler)>;

	void registerFactory(const QString& type, WidgetFactory factory);
	QWidget* build(const QString& type, const WidgetTypeHandler& handler);

private:
	class Private;
	std::unique_ptr<Private> d;
};
