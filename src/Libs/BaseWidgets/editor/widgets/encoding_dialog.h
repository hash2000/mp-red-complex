#pragma once
#include <QDialog>
#include <QString>

class QComboBox;

/// Общий диалог выбора кодировки для редактора кода и просмотрщика больших
/// файлов: одинаковый список, поиск по названию и кнопки действий.
class EncodingDialog final : public QDialog {
public:
	enum class Action {
		None,
		Reopen,
		Resave
	};

	EncodingDialog(const QString& currentEncoding, const QString& hint,
		bool allowResave, QWidget* parent = nullptr);

	QString selectedEncoding() const;
	Action action() const;

private:
	QComboBox* combo = nullptr;
	Action selectedAction = Action::None;
};
