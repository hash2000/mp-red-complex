#pragma once
#include <QWidget>
#include <memory>

class KeyValueEditorWidget : public QWidget {
	Q_OBJECT
public:
	struct Parameter {
		QString name;
		QString value;
		QString description;
		bool isEnabled;
	};

public:
	explicit KeyValueEditorWidget(QWidget* parent = nullptr);
	~KeyValueEditorWidget() override;

	std::vector<Parameter> parameters() const;
	void setParameters(const std::vector<Parameter>& params);

private slots:
	void onToggleAll();
	void onConvert();
	void onAddRow();
	void onRemoveSelectedRow();

private:
	class Private;
	std::unique_ptr<Private> d;
};
