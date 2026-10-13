#pragma once
#include "Content/ConsoleModule/processors/parser/source_cursor.h"
#include <memory>
#include <vector>

using IAstPtr = std::shared_ptr<class IAst>;
using IAstContainer = std::vector<IAstPtr>;

class IAst {
public:
	IAst() = default;
	IAst(const IAst&) = default;
	virtual ~IAst() = default;
	IAst& operator=(const IAst&) = default;

	void setLocation(const SourceLocation& loc) {
		_location = loc;
	}

	SourceLocation location() const {
		return _location;
	}

private:
	SourceLocation _location;
};
