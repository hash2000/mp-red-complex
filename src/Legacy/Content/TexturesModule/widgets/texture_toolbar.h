#pragma once
#include <QWidget>
#include <memory>

enum class ImageType;

class TextureToolbar : public QWidget {
	Q_OBJECT
public:
	explicit TextureToolbar(QWidget* parent = nullptr);
	~TextureToolbar() override;

	// Обновить видимость кнопок в зависимости от типа текстур
	void setImageType(ImageType type);

	// Обновить выбранные тайлы (для управления доступностью кнопок)
	void setSelectedTiles(const QList<int>& tileIds);

signals:
	// Сигналы действий
	void tileSetSettingsRequested();
	void groupsListRequested();
	void groupTilesRequested();
	void ungroupTilesRequested();

private:
	void setupLayout();
	void updateButtonsVisibility();

	class Private;
	std::unique_ptr<Private> d;
};
