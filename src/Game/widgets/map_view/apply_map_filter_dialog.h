#pragma once
#include <QDialog>
#include <memory>

enum class TextureFilter;

struct ApplyMapAtlasSettings {
	TextureFilter textureFilter = TextureFilter::Linear;
	bool useMipMap = false;
	bool useMipMapSmoothing = false;
};

class ApplyMapAtlasDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ApplyMapAtlasDialog(QWidget* parent = nullptr);
	~ApplyMapAtlasDialog() override; // Определение в .cpp обязательно для PIMPL

	// Запрет копирования (стандартная практика для PIMPL)
	ApplyMapAtlasDialog(const ApplyMapAtlasDialog&) = delete;
	ApplyMapAtlasDialog& operator=(const ApplyMapAtlasDialog&) = delete;

	ApplyMapAtlasSettings settings() const;

signals:
	void settingsApplied(TextureFilter filter, bool useMipMap, bool useMipMapSmoothing);

private:
	class Private;
	std::unique_ptr<Private> d;
};
