#pragma once
#include <QDialog>
#include <memory>

class UploadedTextureProperties;

class ApplyMapAtlasDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ApplyMapAtlasDialog(QWidget* parent = nullptr);
	~ApplyMapAtlasDialog() override; // Определение в .cpp обязательно для PIMPL

	// Запрет копирования (стандартная практика для PIMPL)
	ApplyMapAtlasDialog(const ApplyMapAtlasDialog&) = delete;
	ApplyMapAtlasDialog& operator=(const ApplyMapAtlasDialog&) = delete;

	UploadedTextureProperties settings() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
