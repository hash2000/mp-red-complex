#pragma once
#include <QDialog>
#include <QStringList>

#include <memory>

class NameSelectDialog : public QDialog {
	Q_OBJECT;

public:
	NameSelectDialog(const QStringList& variables, QWidget* parent);
	~NameSelectDialog() override;

	QString selectedName() const;

protected:
	void keyPressEvent(QKeyEvent* event) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
