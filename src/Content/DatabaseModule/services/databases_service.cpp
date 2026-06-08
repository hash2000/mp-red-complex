#include "Content/DatabaseModule/services/databases_service.h"

class DatabasesService::Private {
public:
	Private(DatabasesService* parent) : q(parent) {}
	DatabasesService* q;
};

DatabasesService::DatabasesService(QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
}

DatabasesService::~DatabasesService() = default;
