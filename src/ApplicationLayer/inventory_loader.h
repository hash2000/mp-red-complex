#pragma once
#include <QObject>
#include <memory>
#include <QUuid>
#include <QString>

class ItemPlacementService;
class IItemRepository;
class IInventoryRepository;
class IEquipmentRepository;
class ItemsService;

/// Фасад для загрузки инвентарей и экипировок по требованию
/// Загружает данные через репозитории и создаёт сервисы для работы с ними
class InventoryLoader : public QObject {
    Q_OBJECT
public:
    InventoryLoader(
        std::shared_ptr<IInventoryRepository> inventoryRepository,
        std::shared_ptr<IEquipmentRepository> equipmentRepository,
        std::shared_ptr<IItemRepository> itemRepository,
        ItemsService* itemsService,
        QObject* parent = nullptr);
    ~InventoryLoader() override;

    /// Загрузить инвентарь/экипировку по идентификатору и вернуть сервис для работы
    /// @param id Идентификатор инвентаря или экипировки
    /// @return Сервис размещения предметов или nullptr если не найдено
    std::unique_ptr<ItemPlacementService> load(const QUuid& id);

    /// Создать пустой инвентарь в памяти
    /// @param id Идентификатор инвентаря
    /// @param name Название инвентаря
    /// @param rows Количество рядов
    /// @param cols Количество колонок
    /// @return Сервис размещения предметов
    std::unique_ptr<ItemPlacementService> createInventory(const QUuid& id, const QString& name, int rows, int cols);

    /// Создать пустую экипировку в памяти
    /// @param id Идентификатор экипировки
    /// @return Сервис размещения предметов
    std::unique_ptr<ItemPlacementService> createEquipment(const QUuid& id);

private:
    class Private;
    std::unique_ptr<Private> d;
};
