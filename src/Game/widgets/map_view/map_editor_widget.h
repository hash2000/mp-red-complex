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
class TilesService;

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
        MapService* mapService,
        TilesService* tilesService,
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
    void onTileSelected(const QList<int>& tileIds);
    void onMapChanged(const QString& mapName);
		void onApplySelectedAtlas();

protected:
    // Переопределяем обработку кликов
    void onTileClicked(std::optional<QPoint> point) override;
    void onTileHovered(std::optional<QPoint> point) override;
		void onTileServiceConnected() override;

private:
    void setupUI();
    void setupToolbar();
    void setupPropertiesPanel();
    void placeTile(int x, int y);
    void eraseTile(int x, int y);
    void updatePropertiesPanel();

    class Private;
    std::unique_ptr<Private> d;
};
