#pragma once
#include <QObject>

class IMaterialsDataProvider;
class MaterialMimeData;

class MaterialsService : public QObject {
public:
	MaterialsService(IMaterialsDataProvider* materialsDataProvider, QObject* parent = nullptr);
	~MaterialsService() override;

	MaterialMimeData* loadMaterial(const QString& name);

private:
	class Private;
	std::unique_ptr<Private> d;
};
