#pragma once
#include <QWidget>
#include <memory>

class TexturesService;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QComboBox;

class TextureEditorWidget : public QWidget {
	Q_OBJECT
public:
	explicit TextureEditorWidget(TexturesService* texturesService, QWidget* parent = nullptr);
	~TextureEditorWidget() override;

signals:
	void textureSelected(const QString& fileName);

private slots:
	void onTextureTypeChanged(int index);
	void onTextureItemSelected(QListWidgetItem* current);
	void loadTexturesPage();

private:
	void setupLayout();
	void updateTextureList();
	void updatePreview(const QString& fileName);

	class Private;
	std::unique_ptr<Private> d;
};
