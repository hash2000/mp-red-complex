#pragma once
#include <QString>
#include <QStringList>
#include <QHash>
#include <QVariant>
#include <vector>
#include <memory>
#include <optional>

using LazyTreeNodePtr = std::shared_ptr<class ILazyTreeNode>;
using LazyTreeNodeList = std::vector<LazyTreeNodePtr>;

/// Узел дерева с ленивой загрузкой.
///
/// Контракт children():
///  - std::nullopt            — обычный лист, дочерних узлов нет;
///  - пустой список           — дочерние узлы есть, но их нужно загрузить
///                              отдельным запросом при раскрытии узла;
///  - непустой список         — дети уже переданы и используются как есть,
///                              дополнительный запрос при раскрытии не нужен.
class ILazyTreeNode {
public:
	virtual ~ILazyTreeNode() = default;

	virtual void setId(QVariant id) = 0;
	virtual QVariant id() const = 0;
	virtual void setParentId(QVariant parentId) = 0;
	virtual QVariant parentId() const = 0;
	virtual void setName(const QString& name) = 0;
	virtual QString name() const = 0;
	virtual void setExpanded(bool value) = 0;
	virtual bool expanded() const = 0;

	virtual void appendChild(const LazyTreeNodePtr& ch) = 0;
	virtual void setChildren(const LazyTreeNodeList& ch) = 0;
	virtual void clearChildren() = 0;
	virtual std::optional<LazyTreeNodeList> children() const = 0;

	/// Произвольные свойства узла. В базе сохраняются как JSON-объект.
	virtual QVariantMap properties() const = 0;
	virtual bool setProperty(const QString& name, const QVariant& value) = 0;

	/// Удалить свойство вместе с его mime-типами.
	/// Реализация по умолчанию недоступна: поставщик должен её поддержать,
	/// если свойства редактируются.
	virtual bool removeProperty(const QString& name) {
		Q_UNUSED(name);
		return false;
	}

	/// Набор mime-типов свойства. Один mime-тип задаёт редактор значения,
	/// остальные описывают альтернативные представления значения.
	virtual QStringList propertyMimeTypes(const QString& name) const {
		Q_UNUSED(name);
		return {};
	}

	virtual bool setPropertyMimeTypes(const QString& name, const QStringList& mimeTypes) {
		Q_UNUSED(name);
		Q_UNUSED(mimeTypes);
		return false;
	}

	/// Mime-типы всех свойств: имя свойства -> список mime-типов.
	virtual QHash<QString, QStringList> propertyMimeTypes() const {
		QHash<QString, QStringList> result;
		for (const auto& name : properties().keys()) {
			result.insert(name, propertyMimeTypes(name));
		}

		return result;
	}

	/// Хранить значение свойства отдельно, в таблице блоков (`data_blobs`).
	/// Конкретная реализация решает это по mime-типам свойства. По умолчанию
	/// значение пишется в JSON свойства (`value`).
	virtual bool propertyValueIsBlob(const QString& name) const {
		Q_UNUSED(name);
		return false;
	}

	/// GUID блока значения свойства (если значение уже сохранено как блок).
	virtual QString propertyValueBlobId(const QString& name) const {
		Q_UNUSED(name);
		return {};
	}

	/// Запомнить GUID блока значения свойства.
	virtual bool setPropertyValueBlobId(const QString& name, const QString& blobId) {
		Q_UNUSED(name);
		Q_UNUSED(blobId);
		return false;
	}
};
