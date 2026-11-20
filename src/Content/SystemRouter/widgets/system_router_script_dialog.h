#pragma once
#include "Content/SystemRouter/models/system_router_command.h"

#include <QDialog>
#include <QList>

#include <memory>

/// Диалог выбора и предварительного просмотра скрипта узла.
///
/// Слева — список скриптов, справа — название, версия обработчика, описание
/// и собранный код с подставленными параметрами.
class SystemRouterScriptDialog final : public QDialog {
	Q_OBJECT
public:
	explicit SystemRouterScriptDialog(const QList<SystemRouterScriptPreview>& previews,
		QWidget* parent = nullptr);
	~SystemRouterScriptDialog() override;

	const SystemRouterScriptPreview& selectedPreview() const;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
