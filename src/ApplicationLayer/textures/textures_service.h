#pragma once
#include "DataLayer/textures/i_textures_data_provider.h"
#include <QObject>
#include <memory>
#include <QHash>
#include <QPixmap>

class TexturesService : public QObject {
	Q_OBJECT
public:
	explicit TexturesService(
		ITexturesDataProvider* dataProvider,
		QObject* parent = nullptr);
	~TexturesService() override;

	// Получение текстуры (с кэшированием)
	QPixmap getTexture(const QString& path);

	// Очистка кэша
	void clearCache();

	// Предварительная загрузка иконки в кэш
	void preloadTexture(const QString& name);

private:
	class Private;
	std::unique_ptr<Private> d;
};
