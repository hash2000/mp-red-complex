#pragma once
#include <QDialog>
#include <memory>

class TilesService;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
struct TileGroup;

class TileGroupsDialog : public QDialog {
	Q_OBJECT
public:
	explicit TileGroupsDialog(
		TilesService* tilesService,
		const QString& texturePath,
		QWidget* parent = nullptr);
	~TileGroupsDialog() override;

	// Получить выбранную группу (если пользователь нажал "Выбрать")
	std::optional<TileGroup> selectedGroup() const;

private slots:
	void onListItemClicked(QListWidgetItem* item);
	void onListItemDoubleClicked(QListWidgetItem* item);
	void onRenameClicked();
	void onSelectClicked();
	void onDeleteClicked();

private:
	void setupLayout();
	void refreshList();

	class Private;
	std::unique_ptr<Private> d;
};
