#pragma once
#include <exception>
#include <stdexcept>
#include <QString>

class Exception : public std::exception {
public:
	Exception(const QString& msg);

	const char* what() const noexcept override;

private:
	std::runtime_error _error;
};
