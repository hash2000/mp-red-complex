#pragma once
#include <QString>
#include <QStringList>
#include <QWidget>

#include <memory>

class DocumentWidget;
class QShowEvent;

/// Панель «Свойства»: следует за активным документом и показывает его
/// параметры сгруппированными разделами.
///
/// Группы верхнего уровня:
/// - `document` — «Свойства документа»: редактор документа и его
///   дополнительные группы (например, «Параметры поиска»);
/// - `tab` — «Свойства вкладки»: заголовок, значок, цвет, тип, изменённость;
/// - `context` — «Параметры контекста»: полное имя контекста и вложенная
///   группа `context.variables` с видимыми переменными.
///
/// Список переменных контекста строится лениво — только при раскрытии группы.
class PropertiesPanel : public QWidget {
	Q_OBJECT
public:
	explicit PropertiesPanel(QWidget* parent = nullptr);
	~PropertiesPanel() override;

	void setDocument(DocumentWidget* doc);
	DocumentWidget* document() const;

	/// Раскрыть группу свойств (например, "search", "tab", "context.variables")
	/// и прокрутить панель к ней. Неизвестная группа игнорируется.
	void revealGroup(const QString& groupId);
	bool isGroupExpanded(const QString& groupId) const;
	QStringList groupIds() const;

signals:
	/// Группа раскрыта (программно или пользователем).
	void groupExpanded(const QString& groupId);

protected:
	void showEvent(QShowEvent* event) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
