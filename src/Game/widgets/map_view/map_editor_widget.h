#pragma once
#include "Game/widgets/map_view/map_view_base.h"
#include <QWidget>
#include <QList>
#include <memory>

class QToolBar;
class QComboBox;
class QLabel;
class QSpinBox;
class QGroupBox;
class QPushButton;
class MapService;
class TilesSelectorService;
class TexturesService;


// Режимы редактирования карты
enum class MapEditorMode {
  Draw,       // Рисование тайлами
  Erase,      // Стирание
  Select,     // Выделение области
  Properties  // Редактирование свойств тайлов
};

class MapEditorWidget : public MapViewBase {
  Q_OBJECT
public:
  explicit MapEditorWidget(
		TexturesService* textureService,
		MapService* mapService,
		TilesSelectorService* tilesSelectorService,
		QWidget* parent = nullptr);

  ~MapEditorWidget() override;

  // Получить текущий режим
  MapEditorMode currentMode() const;

  // Установить режим
  void setMode(MapEditorMode mode);

signals:
  void modeChanged(MapEditorMode mode);
  void tilesPlaced(int x, int y, const QList<int>& tileIds);
  void tileErased(int x, int y);
  void areaSelected(QRect area);

private slots:
  void onModeChanged(int index);
  void onMapChanged(const QString& mapName);
  void onApplySelectedAtlas();
  void onBeginFrame();
  void onTileClicked(std::optional<QPoint> point);
  void onTileHovered(std::optional<QPoint> point);
	void onRenderLevelEditChanged(const QString& text);
	void onShowSelectedLayerOnly(bool checked);
	void onSaveMap();

private:
  void setupUI();
  void setupToolbar();
  void setupPropertiesPanel();
  void placeTile(int x, int y);
  void eraseTile(int x, int y);
  void updatePropertiesPanel();
  void selectTile(int x, int y);

	void setTileRenderLayer(int value);
	int tileRenderLayer() const;


  class Private;
  std::unique_ptr<Private> d;
};
