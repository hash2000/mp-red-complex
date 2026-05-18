#pragma once
#include <QWidget>
#include <memory>

enum class MaterialObjectsAddType {
	VertexShader,
	FragmentShader,
	Directory,
	Texture
};

class MaterialObjects : public QWidget {
	Q_OBJECT
public:
	MaterialObjects(QWidget* parent = nullptr);
	~MaterialObjects() override;

signals:
	void addRequested(MaterialObjectsAddType type);
	void refreshRequested();
	void loadRequested();
	void saveRequested();

private slots:
	void onCustomContextMenuRequested(const QPoint& pos);
	void onItemDoubleClicked(const QModelIndex& index);

private:
	void setupUI();

public:
	class Private;
	std::unique_ptr<Private> d;
};
