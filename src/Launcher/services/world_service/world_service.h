#pragma once
#include <QObject>
#include <memory>

class WorldService : public QObject {
	Q_OBJECT
public:
	explicit WorldService(QObject* parent = nullptr);
	~WorldService() override;

public slots:
	void onSave();
	void onLoad();

private:
	class Private;
	std::unique_ptr<Private> d;
};
