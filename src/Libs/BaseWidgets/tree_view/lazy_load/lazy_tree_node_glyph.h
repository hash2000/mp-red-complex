#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

/// Глиф узла дерева — короткий символ, который представление рисует перед
/// именем узла.
///
/// Глиф хранится в свойствах узла (`properties()`):
///  - `glyph` — основное имя свойства;
///  - `icon` — прежнее имя, оно встречается в уже сохранённых данных.
/// Если глиф не назначен, представление рисует defaultGlyph().
namespace LazyTreeNodeGlyph {

/// Имя свойства узла с глифом.
inline QString propertyName() {
	return QStringLiteral("glyph");
}

/// Прежнее имя того же свойства.
inline QString legacyPropertyName() {
	return QStringLiteral("icon");
}

/// Глиф для узла, которому глиф не назначен.
inline QString defaultGlyph() {
	return QStringLiteral("📄");
}

/// Является ли свойство узла свойством глифа (`glyph` или прежнее `icon`).
inline bool isProperty(const QString& name) {
	return name.compare(propertyName(), Qt::CaseInsensitive) == 0
		|| name.compare(legacyPropertyName(), Qt::CaseInsensitive) == 0;
}

/// Назначенный узлу глиф; пустая строка — глиф не назначен.
inline QString assigned(const LazyTreeNodePtr& node) {
	if (!node) {
		return {};
	}

	const auto values = node->properties();
	auto glyph = values.value(propertyName()).toString().trimmed();
	if (glyph.isEmpty()) {
		glyph = values.value(legacyPropertyName()).toString().trimmed();
	}

	return glyph;
}

/// Глиф для отображения: назначенный или defaultGlyph().
inline QString resolved(const LazyTreeNodePtr& node) {
	const auto glyph = assigned(node);
	return glyph.isEmpty() ? defaultGlyph() : glyph;
}

/// Назначить глиф. Пустая строка удаляет назначение — узел снова получает
/// глиф по умолчанию. false — узел не задан.
inline bool assign(const LazyTreeNodePtr& node, const QString& glyph) {
	if (!node) {
		return false;
	}

	// Прежнее свойство убираем: иначе оно перекрыло бы новое назначение.
	node->removeProperty(legacyPropertyName());

	const auto trimmed = glyph.trimmed();
	if (trimmed.isEmpty()) {
		node->removeProperty(propertyName());
		return true;
	}

	return node->setProperty(propertyName(), trimmed);
}

} // namespace LazyTreeNodeGlyph
