#pragma once 
#include <QWidget>
#include <memory>

class MaterialVariables : public QWidget {
	Q_OBJECT
public:
	MaterialVariables(QWidget* parent = nullptr);
	~MaterialVariables() override;

private:
	void setupUI();

public:
	class Private;
	std::unique_ptr<Private> d;
};
