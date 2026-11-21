#pragma once
#include <QStringList>
#include <QWidget>
#include <memory>

class CommandContext;
class DocumentWidget;

/// Панель открытых документов слева.
///
/// Порядок и группировку вкладок определяет правило сортировки
/// (IDocumentTabsOrderStrategy): ручная сортировка с перетаскиванием мышью,
/// группировка по цвету, по типу документа. Активное правило берётся из
/// системного параметра `documents.tabs.order` (переменная глобального
/// контекста), поэтому его можно менять в настройках приложения.
class DocumentTabsPanel : public QWidget {
	Q_OBJECT
public:
	explicit DocumentTabsPanel(QWidget* parent = nullptr);
	/// @param settingsContext контекст системных настроек (обычно глобальный):
	/// из него читается правило сортировки/группировки вкладок.
	explicit DocumentTabsPanel(std::shared_ptr<CommandContext> settingsContext,
		QWidget* parent = nullptr);
	~DocumentTabsPanel() override;

	void addDocument(DocumentWidget* doc);
	void removeDocument(DocumentWidget* doc);
	void setActiveDocument(DocumentWidget* doc);

	/// Документы в порядке ручной сортировки (без учёта группировки).
	QList<DocumentWidget*> documents() const;

	/// Перемещает документ на позицию ручной сортировки (0..размер-1).
	/// false — документа нет в панели.
	bool moveDocument(DocumentWidget* doc, int index);

	/// Идентификатор активного правила сортировки/группировки.
	QString orderStrategy() const;
	/// Включает правило по идентификатору; false — правило не найдено.
	bool setOrderStrategy(const QString& id);

	/// Заголовки групп, построенных последней перестройкой (для тестов
	/// и диагностики); пустой список — панель без групп.
	QStringList groupTitles() const;

signals:
	void activateRequested(DocumentWidget* doc);
	void closeRequested(DocumentWidget* doc);
	void propertiesRequested(DocumentWidget* doc);
	void newDocumentRequested();

private:
	struct Private;
	std::unique_ptr<Private> d;
};
