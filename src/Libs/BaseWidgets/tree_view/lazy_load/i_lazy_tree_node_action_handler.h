#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <QString>
#include <QVariant>
#include <QStandardItem>

class ILazyTreeNodeActionHandler {
public:
	virtual ~ILazyTreeNodeActionHandler() = default;

	struct ActionResult {
		bool success;
		QString errorMessage; // Пустая строка, если success == true

		static ActionResult Ok() {
			return { true, "" };
		}
		static ActionResult Error(const QString& msg) {
			return { false, msg };
		}
	};

	// Создание узла
	// parentItem - родитель (может быть invisibleRootItem)
	// newNode - узел с заполненным именем, но без ID
	// После успешного выполнения newNode должен иметь присвоенный ID
	virtual ActionResult addNode(QStandardItem* parentItem, LazyTreeNodePtr newNode) = 0;

	// Удаление узла
	// nodeItem - удаляемый узел
	virtual ActionResult removeNode(QStandardItem* nodeItem) = 0;

	// Переименование узла
	// nodeItem - переименуемый узел
	// newName - новое имя
	virtual ActionResult renameNode(QStandardItem* nodeItem, const QString& newName) = 0;

	// Перемещение узла (drag & drop)
	// nodeItem - перемещаемый узел
	// newParentItem - новый родитель
	virtual ActionResult moveNode(QStandardItem* nodeItem, QStandardItem* newParentItem) = 0;
};
