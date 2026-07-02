#pragma once
#include <QObject>

class FormatterPlugin: public QObject {
	Q_OBJECT
public:
	virtual ~FormatterPlugin() = default;
	virtual QStringList mimeTypes() const = 0;
	virtual void formatAsync(const QString& text) = 0;
	virtual void stop() = 0;

signals:
	void formattingFinished(const QString& formattedText);
	void formattingFailed(const QString& errorMessage);
};

class FormatterPluginAsync : public FormatterPlugin {
	Q_OBJECT
public:
	FormatterPluginAsync();
	~FormatterPluginAsync() override;

	void formatAsync(const QString& text) override;
	void stop() override;

protected:
	virtual QString internalFormat(const QString& inputText) = 0;

private:
	class Private;
	std::unique_ptr<Private> d;
};
