#pragma once
#include <QWidget>
#include <memory>

class MaterialObjects : public QWidget {
	Q_OBJECT
public:
	MaterialObjects(QWidget* parent = nullptr);
	~MaterialObjects() override;

private:
	void setupUI();

public:
	class Private;
	std::unique_ptr<Private> d;
};
