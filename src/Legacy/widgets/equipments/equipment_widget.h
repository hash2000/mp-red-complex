#pragma once
#include <QFrame>
#include <QUuid>
#include <memory>

enum class ItemSlotType;
class ContainerItem;
class InventoriesService;
class ItemMimeData;

class EquipmentWidget : public QFrame {
	Q_OBJECT

public:
	explicit EquipmentWidget(InventoriesService* inventoriesService, QWidget* parent = nullptr);
	~EquipmentWidget() override;

	bool setEquipmentService(const QUuid& id);

private slots:
	void onItemEquipped(const ContainerItem& item, ItemSlotType slot, const QUuid& inventoryId);
	void onItemUnequipped(const ContainerItem& item, ItemSlotType slot);

signals:
	void containerOpened(const ItemMimeData& container);

private:
	class Private;
	std::unique_ptr<Private> d;
};
