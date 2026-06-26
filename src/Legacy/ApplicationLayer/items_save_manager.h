#pragma once
#include <QObject>
#include <memory>

class ItemsService;
class IItemsDataWriter;

/**
 * @brief Менеджер сохранения предметов
 *
 * Подключается к сигналу save() из Services и сохраняет
 * все созданные предметы через DataWriter
 */
class ItemsSaveManager : public QObject {
	Q_OBJECT
public:
	ItemsSaveManager(
		ItemsService* itemsService,
		IItemsDataWriter* itemsDataWriter,
		QObject* parent = nullptr);

	~ItemsSaveManager() override;

public slots:
	void saveAll();

private:
	class Private;
	std::unique_ptr<Private> d;
};
