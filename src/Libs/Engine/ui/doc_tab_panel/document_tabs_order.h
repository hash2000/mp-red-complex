#pragma once
#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>
#include <memory>

class DocumentWidget;
class SystemOptionsRegistry;

/// Вкладка с учётом темы: документ и цвет, который тема задаёт его классу.
///
/// Цвет вкладки складывается из двух источников: явного цвета документа
/// (`DocumentWidget::color`, редактируется в свойствах) и цвета темы для
/// класса документа или его xtype (см. DocumentTabTheme). Явный цвет важнее.
struct DocumentTabView {
	DocumentWidget* document = nullptr;
	/// Цвет из темы; невалидный — привязки для документа в теме нет.
	QColor themeColor;
};

/// Действующий цвет вкладки: явный цвет документа важнее цвета темы.
QColor documentTabColor(const DocumentTabView& view);

/// Группа вкладок: заголовок (пустой — группа без заголовка), цвет группы
/// и вкладки в порядке отображения.
struct DocumentTabGroup {
	QString title;
	QColor color;
	QList<DocumentTabView> tabs;
};

/// Правило сортировки/группировки вкладок документов. Одновременно действует
/// одно правило: его идентификатор хранится в системном параметре
/// `documents.tabs.order`, а допустимые значения собираются из реестра правил.
///
/// Чтобы добавить вариант (например, группировку по проекту), достаточно
/// реализовать интерфейс и зарегистрировать правило в DocumentTabsOrderRegistry:
/// настройка, интерфейс настроек и панель вкладок подхватят его сами.
class IDocumentTabsOrderStrategy {
public:
	virtual ~IDocumentTabsOrderStrategy() = default;

	/// Идентификатор правила — значение системного параметра.
	virtual QString id() const = 0;
	/// Подпись правила для интерфейса настроек.
	virtual QString title() const = 0;
	/// Разрешено ли перетаскивание вкладок мышью (ручная сортировка).
	virtual bool supportsManualMove() const = 0;
	/// Порядок и группировка вкладок.
	/// @param tabs вкладки в порядке ручной сортировки
	virtual QList<DocumentTabGroup> arrange(const QList<DocumentTabView>& tabs) const = 0;
};

/// Реестр правил сортировки/группировки вкладок.
/// Встроенные правила регистрируются в конструкторе.
class DocumentTabsOrderRegistry {
public:
	DocumentTabsOrderRegistry();
	~DocumentTabsOrderRegistry();

	void registerStrategy(std::unique_ptr<IDocumentTabsOrderStrategy> strategy);
	/// Зарегистрированные правила в порядке регистрации.
	QList<IDocumentTabsOrderStrategy*> strategies() const;
	IDocumentTabsOrderStrategy* strategy(const QString& id) const;
	QStringList ids() const;
	/// Идентификатор правила по умолчанию (ручная сортировка).
	QString defaultId() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};

/// Регистрирует системный параметр сортировки вкладок: значения параметра —
/// идентификаторы зарегистрированных правил, подписи — их названия.
void registerDocumentTabsOrderOption(SystemOptionsRegistry& registry);
