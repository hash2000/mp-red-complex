#pragma once
#include <QObject>
#include <memory>
#include <QUuid>
#include <QString>

class IItemPlacementService;
class IItemRepository;
class IInventoryRepository;
class IEquipmentRepository;
class ItemsService;
class ItemContainer;

/// Фасад для загрузки инвентарей и экипировок по требованию
/// Загружает данные через репозитории и создаёт сервисы для работы с ними
class InventoryLoader : public QObject {
    Q_OBJECT
public:
    InventoryLoader(
       IInventoryRepository* inventoryRepository,
       IEquipmentRepository* equipmentRepository,
       IItemRepository* itemRepository,
       ItemsService* itemsService,
       QObject* parent = nullptr);

    ~InventoryLoader() override;

    /// Загрузить инвентарь/экипировку по идентификатору и вернуть сервис для работы
    /// @param id Идентификатор инвентаря или экипировки
    /// @return Сервис размещения предметов или nullptr если не найдено
    IItemPlacementService* load(const QUuid& id);

    /// Создать пустой инвентарь в памяти
    /// @param id Идентификатор инвентаря
    /// @param name Название инвентаря
    /// @param rows Количество рядов
    /// @param cols Количество колонок
    /// @return Сервис размещения предметов
    IItemPlacementService* createInventory(const QUuid& id, const QString& name, const ItemContainer& contaiter);

    /// Создать пустую экипировку в памяти
    /// @param id Идентификатор экипировки
    /// @return Сервис размещения предметов
    IItemPlacementService* createEquipment(const QUuid& id);

private:
    class Private;
    std::unique_ptr<Private> d;
};
