#pragma once
#include <QString>
#include <QWidget>
#include <memory>
#include <functional>

class CommandContext;

struct WidgetTypeHandler {
	QString className;
	QString xtype;
};

struct WidgetBuildHandler : public WidgetTypeHandler {
	QString name;
	QString title;
	QString glyph;
	QWidget* parent;
	std::shared_ptr<CommandContext> context;
};

class WidgetRegistry {
public:
	WidgetRegistry();
	virtual ~WidgetRegistry();

	using WidgetFactory = std::function<QWidget*(const QString& type, const WidgetBuildHandler& handler)>;

	void registerFactory(const WidgetTypeHandler& type, WidgetFactory factory);
	QWidget* build(const QString& type, const WidgetBuildHandler& handler);

private:
	class Private;
	std::unique_ptr<Private> d;
};
