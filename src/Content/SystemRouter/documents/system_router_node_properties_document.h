#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <QStringList>
#include <QVariant>
#include <memory>

class PropertiesListWidget;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QToolButton;
class SystemRouterBlobService;
class SystemRouterService;

/// Документ параметров узла системного маршрутизатора.
///
/// Показывает `ILazyTreeNode::properties()` в PropertiesListWidget и позволяет
/// добавлять, удалять и изменять свойства вместе с их mime-типами.
/// Изменения сразу сохраняются через SystemRouterService.
///
/// Отдельно показываются параметры, которыми управляет представление:
/// «Узел всегда открыт» (поле expanded таблицы узлов) и «Глиф» (свойство
/// `glyph` — символ перед именем узла в дереве).
class SystemRouterNodePropertiesDocument final : public DocumentWidget {
	Q_OBJECT
public:
	/// @param nodeId идентификатор узла; пустой — узел будет прочитан из
	///        переменной контекста документа `nodeId` при привязке контекста.
	explicit SystemRouterNodePropertiesDocument(SystemRouterService* service,
		SystemRouterBlobService* blobService = nullptr, const QVariant& nodeId = {},
		QWidget* parent = nullptr);
	~SystemRouterNodePropertiesDocument() override;

	QVariant nodeId() const;
	LazyTreeNodePtr node() const;

	/// Признак «узел всегда открыт».
	bool alwaysExpanded() const;
	bool setAlwaysExpanded(bool value);

	/// Глиф узла; пустая строка — глиф по умолчанию.
	QString glyph() const;
	bool setGlyph(const QString& glyph);

	bool addProperty(const QString& name, const QStringList& mimeTypes,
		const QVariant& value = {});
	bool removeProperty(const QString& name);
	bool setPropertyValue(const QString& name, const QVariant& value);
	bool setPropertyMimeTypes(const QString& name, const QStringList& mimeTypes);

	void refresh() override;
	bool save() override;

signals:
	void errorOccurred(const QString& message);

protected:
	void onContextAttached() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
