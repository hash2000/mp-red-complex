#pragma once
#include "Launcher/exception.h"

class ApplicationExceptionUndefinedMainFrame : public Exception {
public:
	ApplicationExceptionUndefinedMainFrame()
	: Exception("Undefined MainFrame") {
	}
};
