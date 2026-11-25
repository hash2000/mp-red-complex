#include "Content/TextEditorModule/documents/text_document_options.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/system_options/system_option.h"

namespace {

QVariant optionValue(const std::shared_ptr<CommandContext>& context,
	const char* name, const QVariant& fallback) {
	if (!context) {
		return fallback;
	}

	// В режиме Lazy документ видит переменные глобального контекста, если
	// не переопределил их у себя.
	return context->variable(QString::fromLatin1(name), fallback);
}

} // namespace

TextDocumentOptions TextDocumentOptions::fromContext(
	const std::shared_ptr<CommandContext>& context) {
	TextDocumentOptions options;
	options.fontFamily = optionValue(context,
		SystemOptions::kDocumentsTextFontFamily, QString()).toString();
	options.fontSize = qMax(0, optionValue(context,
		SystemOptions::kDocumentsTextFontSize, 0).toInt());
	options.wordWrap = optionValue(context,
		SystemOptions::kDocumentsTextWordWrap, false).toBool();
	options.encoding = optionValue(context,
		SystemOptions::kDocumentsTextEncoding,
		QString::fromLatin1(SystemOptions::kDocumentsTextEncodingDefault)).toString();
	options.bufferLimitMb = qMax(0, optionValue(context,
		SystemOptions::kDocumentsTextBufferLimit,
		SystemOptions::kDocumentsTextBufferLimitDefaultMb).toInt());
	return options;
}
