#pragma once
#include "Base/exception.h"

class ApplicationExceptionUndefinedMainFrame : public Exception {
public:
	ApplicationExceptionUndefinedMainFrame()
	: Exception("Undefined MainFrame") {
	}
};
