#pragma once
#include <QObject>
#include <memory>

class DatabasesService : public QObject {
	Q_OBJECT
public:
	DatabasesService(QObject* parent = nullptr);
	~DatabasesService();


private:
	class Private;
	std::unique_ptr<Private> d;
};
