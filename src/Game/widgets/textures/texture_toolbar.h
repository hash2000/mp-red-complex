#pragma once
#include <QWidget>
#include <memory>

enum class TextureType;

class TextureToolbar : public QWidget {
	Q_OBJECT
public:
	explicit TextureToolbar(QWidget* parent = nullptr);
	~TextureToolbar() override;

	// Обновить видимость кнопок в зависимости от типа текстур
	void setTextureType(TextureType type);

signals:
	// Сигналы действий
	void tileSetSettingsRequested();
	// В будущем здесь будут сигналы для других действий с тайлами
	// void tileGroupCreateRequested();
	// void tileGroupDeleteRequested();

private:
	void setupLayout();
	void updateButtonsVisibility();

	class Private;
	std::unique_ptr<Private> d;
};
