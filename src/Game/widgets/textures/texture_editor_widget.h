#pragma once
#include <QWidget>
#include <memory>

class TexturesService;
class QListWidget;
class QListWidgetItem;
class QComboBox;
class TextureToolbar;
class TileSetParamsPanel;
enum class TextureType;

class TextureEditorWidget : public QWidget {
	Q_OBJECT
public:
	explicit TextureEditorWidget(TexturesService* texturesService, QWidget* parent = nullptr);
	~TextureEditorWidget() override;

signals:
	void textureSelected(TextureType textureType, const QString& fileName);
	void tileSelected(int tileId);

private slots:
	void onTextureTypeChanged(int index);
	void onTextureItemSelected(QListWidgetItem* current);
	void loadTexturesPage();
	void onTileSetSettingsRequested();
	void onTileSetSettingsApplied(int gridSizeX, int gridSizeY, bool showGrid);
	void onTileClicked(int tileId);

private:
	void setupLayout();
	void updateTextureList();
	void updatePreview(const QString& fileName);

	class Private;
	std::unique_ptr<Private> d;
};
