#pragma once
#include <QWidget>
#include <memory>
#include <map>

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

	std::map<QString, QString> parametersMap(bool enabledOnly = true) const;
	std::vector<Parameter> parameters() const;
	void setParameters(const std::vector<Parameter>& params);
	void applyParameters(const std::vector<Parameter>& params);

	void setReadonly(bool set = true);

private slots:
	void onToggleAll();
	void onConvert();
	void onAddRow();
	void onRemoveSelectedRow();

private:
	class Private;
	std::unique_ptr<Private> d;
};
