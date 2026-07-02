#pragma once

struct LanguageInfo;

class FormatterPlugin {
public:
	virtual ~FormatterPlugin() = default;
	virtual LanguageInfo languageInfo() const = 0;
};
