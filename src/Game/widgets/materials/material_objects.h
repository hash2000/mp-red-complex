#pragma once
#include "Game/widgets/materials/material_objects/material_object_node.h"
#include <QWidget>
#include <memory>

class MaterialsService;

class MaterialObjects : public QWidget {
	Q_OBJECT
public:
	MaterialObjects(MaterialsService* materialsService, QWidget* parent = nullptr);
	~MaterialObjects() override;

signals:
	void refreshRequested();
	void loadRequested();
	void saveRequested();

private slots:
	void onItemSelected(const QModelIndex& current, const QModelIndex& previous);
	void onItemDoubleClicked(const QModelIndex& index);
	void onAddSubButtonTriggered(MaterialObjectTypes type);
	void onPropertyChanged(const QString& propertyId, const QVariant& newValue);
	void onPropertyButtonClick(const QString& propertyId);
	void onCustomContextMenuRequested(const QPoint& pos);

private:
	void setupUI();
	void updateProperties();

public:
	class Private;
	std::unique_ptr<Private> d;
};
