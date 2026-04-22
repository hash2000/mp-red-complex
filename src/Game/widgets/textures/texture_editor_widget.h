#pragma once
#include <QWidget>
#include <memory>

class ImagesService;
class TilesService;
class QListWidget;
class QListWidgetItem;
class QComboBox;
class TextureToolbar;
class TileSetParamsDialog;
enum class ImageType;

class TextureEditorWidget : public QWidget {
	Q_OBJECT
public:
	explicit TextureEditorWidget(
		ImagesService* ImagesService,
		TilesService* tilesService,
		QWidget* parent = nullptr);
	~TextureEditorWidget() override;

signals:
	void textureSelected(ImageType ImageType, const QString& fileName);
	void tileSelected(int tileId);

private slots:
	void onImageTypeChanged(int index);
	void onTextureItemSelected(QListWidgetItem* current);
	void loadsPage();
	void onTileSetSettingsRequested();
	void onTileSetSettingsApplied(int gridSizeX, int gridSizeY, bool showGrid);
	void onTileGroupsRequested();
	void onTileClicked(int tileId, bool ctrlModifier);
	void onGroupTilesRequested();
	void onUngroupTilesRequested();
	void onGroupsChanged(const QString& texturePath);

private:
	void setupLayout();
	void updateTextureList();
	void updatePreview(const QString& fileName);
	void updateSelectedTiles(int tileId);

	class Private;
	std::unique_ptr<Private> d;
};
