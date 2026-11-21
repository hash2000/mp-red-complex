#pragma once
#include "Libs/Engine/scripting/command_context/document_rule.h"

#include <QList>
#include <QString>
#include <QWidget>
#include <functional>
#include <memory>
#include <optional>

class CommandContext;
class DocumentWidget;
class IDocumentMetadata;

/// Регистрационное описание типа виджета. Тип может быть создан как
/// документ или панель через MainWindow::createWidget.
struct WidgetTypeHandler {
	WidgetTypeHandler() = default;
	/// @param className полное имя класса (для справки и поиска в реестре)
	/// @param xtype псевдоним типа, которым оперируют описания и команды
	/// @param singletone обязательный признак: такой документ создаётся
	///        один на приложение, а его имя (id) всегда равно xtype
	/// @param glyph иконка UTF-8 по умолчанию
	/// @param title подпись по умолчанию
	/// @param rule роль документа: набор правил контекста и привилегий
	///        (по умолчанию — обычный документ, текущее поведение)
	WidgetTypeHandler(const QString& className, const QString& xtype, bool singletone,
		const QString& glyph = QString(), const QString& title = QString(),
		DocumentRule rule = DocumentRule::Document);

	QString className;
	QString xtype;
	bool singletone = false;
	QString glyph;
	QString title;
	DocumentRule rule = DocumentRule::Document;
};

/// Параметры конкретного создания: описание типа плюс экземплярные данные.
struct WidgetBuildHandler : public WidgetTypeHandler {
	using WidgetTypeHandler::WidgetTypeHandler;

	QString name;
	QWidget* parent = nullptr;
	QVariantMap options;
	std::shared_ptr<CommandContext> context;
};

class WidgetRegistry {
public:
	WidgetRegistry();
	virtual ~WidgetRegistry();

	using WidgetFactory = std::function<QWidget*(const QString& type, const WidgetBuildHandler& handler)>;

	/// Регистрирует фабрику типа. singletone, glyph и title берутся из описания.
	/// Пустой className/xtype и повторная регистрация отклоняются с qWarning().
	void registerFactory(const WidgetTypeHandler& type, WidgetFactory factory);

	/// Зарегистрированные типы (по одному на xtype), отсортированы по xtype.
	QList<WidgetTypeHandler> types() const;

	/// Описание типа по xtype или полному имени класса.
	std::optional<WidgetTypeHandler> typeInfo(const QString& type) const;

	/// Идентификатор создаваемого документа: для singletone — всегда xtype,
	/// иначе предложенный id. Пустой тип не меняет id.
	QString documentId(const QString& type, const QString& requestedId) const;

	/// Регистрирует метаданные типа документа по полному имени класса.
	/// Повторная регистрация и пустые данные отклоняются с qWarning().
	void registerDocumentMetadata(const QString& className, std::unique_ptr<IDocumentMetadata> metadata);

	/// Метаданные типа по xtype или полному имени класса; nullptr — не заданы.
	const IDocumentMetadata* documentMetadata(const QString& type) const;

	/// Создаёт виджет без обёртки-документа. Для вьюпорта и служебных
	/// виджетов, которые не являются документами.
	QWidget* buildWidget(const QString& type, const WidgetBuildHandler& handler);

	/// Создаёт документ: результат фабрики оборачивается в DocumentWidgetStub,
	/// если фабрика вернула не DocumentWidget.
	DocumentWidget* build(const QString& type, const WidgetBuildHandler& handler);

private:
	class Private;
	std::unique_ptr<Private> d;
};
