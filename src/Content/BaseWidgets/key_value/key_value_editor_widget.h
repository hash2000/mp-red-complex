#pragma once
#include <QWidget>
#include <memory>

class KeyValueEditorWidget : public QWidget {
	Q_OBJECT
public:
	explicit KeyValueEditorWidget(QWidget* parent = nullptr);
	~KeyValueEditorWidget() override;

private slots:
	void onToggleAll();
	void onConvert();
	void onAddRow();
	void onRemoveSelectedRow();

private:
	class Private;
	std::unique_ptr<Private> d;
};
