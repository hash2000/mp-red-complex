#include "Libs/Engine/scripting/command_context/context_interceptor_factory.h"
#include "Libs/Engine/scripting/command_context/context_interceptor.h"

#include <QDebug>

std::unique_ptr<IContextInterceptor> createContextInterceptor(DocumentRule rule) {
	switch (rule) {
	case DocumentRule::Document:
		return std::make_unique<StandardContextInterceptor>();
	case DocumentRule::SystemOptions:
		return std::make_unique<SystemOptionsContextInterceptor>();
	}

	qWarning() << "createContextInterceptor: unknown document rule" << static_cast<int>(rule);
	return nullptr;
}
