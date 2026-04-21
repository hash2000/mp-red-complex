#pragma once
#include <QDialog>
#include <memory>

class TileSetParamsDialog : public QDialog {
	Q_OBJECT
public:
	explicit TileSetParamsDialog(int tilesCountX, int tilesCountY, QWidget* parent = nullptr);
	~TileSetParamsDialog() override;

	int gridSizeX() const;
	int gridSizeY() const;
	bool showGrid() const;

signals:
	void settingsApplied(int gridSizeX, int gridSizeY, bool showGrid);

private slots:
	void onApplyClicked();

private:
	void setupLayout();

	class Private;
	std::unique_ptr<Private> d;
};
