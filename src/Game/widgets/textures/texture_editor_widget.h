#pragma once
#include <QWidget>
#include <memory>

class TexturesService;
class QListWidget;
class QListWidgetItem;
class QComboBox;
class QPushButton;
class TileSetParamsPanel;

class TextureEditorWidget : public QWidget {
	Q_OBJECT
public:
	explicit TextureEditorWidget(TexturesService* texturesService, QWidget* parent = nullptr);
	~TextureEditorWidget() override;

signals:
	void textureSelected(const QString& fileName);
	void tileSelected(int tileId);

private slots:
	void onTextureTypeChanged(int index);
	void onTextureItemSelected(QListWidgetItem* current);
	void loadTexturesPage();
	void openTileSetSettings();
	void onTileSetSettingsApplied(int gridSizeX, int gridSizeY, bool showGrid);
	void onTileClicked(int tileId);

private:
	void setupLayout();
	void updateTextureList();
	void updatePreview(const QString& fileName);
	void updateTileSetButton();

	class Private;
	std::unique_ptr<Private> d;
};
