#pragma once
#include <QObject>

class IMaterialsDataProvider;

class MaterialsService : public QObject {
public:
	MaterialsService(IMaterialsDataProvider* materialsDataProvider, QObject* parent = nullptr);
	~MaterialsService() override;


private:
	class Private;
	std::unique_ptr<Private> d;
};
