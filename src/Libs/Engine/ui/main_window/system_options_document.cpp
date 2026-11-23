#include "Libs/Engine/ui/main_window/system_options_document.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/system_options/system_option.h"
#include "Libs/Engine/scripting/system_options/system_options_registry.h"

#include <QColor>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <optional>

namespace {

/// Реестр системных параметров приложения либо nullptr.
SystemOptionsRegistry* systemOptionsOf(const DocumentWidget& document) {
	const auto context = document.context();
	auto* controller = context ? context->commandController() : nullptr;
	return controller ? controller->systemOptions() : nullptr;
}

QColor colorOf(const QVariant& value) {
	if (value.canConvert<QColor>()) {
		return value.value<QColor>();
	}

	return QColor(value.toString());
}

/// Подпись значения Enum (по идентификатору) для выпадающего списка.
QString optionValueTitle(const SystemOption& option, const QString& value) {
	const int index = option.values.indexOf(value);
	if (index < 0) {
		return value;
	}

	return index < option.valueTitles.size() ? option.valueTitles.at(index) : value;
}

/// Идентификатор значения Enum по подписи из выпадающего списка.
QString optionValueFromTitle(const SystemOption& option, const QString& title) {
	const int index = option.valueTitles.indexOf(title);
	if (index >= 0 && index < option.values.size()) {
		return option.values.at(index);
	}

	return title;
}

/// Описание редакторов параметров для PropertiesListWidget.
QList<PropertyData> optionProperties(DocumentWidget& document) {
	QList<PropertyData> result;
	auto* registry = systemOptionsOf(document);
	if (!registry) {
		return result;
	}

	for (const auto& option : registry->options()) {
		PropertyData data;
		data.id = option.name;
		data.label = option.title.isEmpty() ? option.name : option.title;
		data.tooltip = option.description.isEmpty() ? option.name
			: QString("%1\n%2").arg(option.description, option.name);
		data.defaultValue = option.defaultValue;

		// Текущее значение хранится в переменной контекста; если её нет,
		// показываем описанное значение по умолчанию.
		const QVariant current = document.hasVariable(option.name)
			? document.variable(option.name)
			: option.defaultValue;

		switch (option.type) {
		case SystemOptionType::Boolean:
			data.type = PropertyType::Boolean;
			data.value = current.toBool();
			break;
		case SystemOptionType::Integer:
			data.type = PropertyType::Number;
			data.value = current.toInt();
			data.minVal = -1'000'000;
			data.maxVal = 1'000'000;
			data.step = 1;
			break;
		case SystemOptionType::Color:
			data.type = PropertyType::Color;
			data.value = colorOf(current);
			break;
		case SystemOptionType::Enum:
			data.type = PropertyType::Combo;
			data.comboOptions = option.valueTitles.isEmpty() ? option.values : option.valueTitles;
			data.value = optionValueTitle(option, current.toString());
			break;
		case SystemOptionType::Text:
			data.type = PropertyType::Text;
			data.value = current.toString();
			break;
		}

		result << data;
	}

	return result;
}

bool isOptionName(const SystemOptionsRegistry* registry, const QString& name) {
	return registry && registry->option(name).has_value();
}

} // namespace

const QString& SystemOptionsDocument::xtype() {
	static const QString value = QStringLiteral("system-options");
	return value;
}

const QString& SystemOptionsDocument::className() {
	static const QString value = QStringLiteral("System.Documents.SystemOptions");
	return value;
}

SystemOptionsDocument::SystemOptionsDocument(QWidget* parent)
	: DocumentWidget(parent) {
	setObjectName(QStringLiteral("SystemOptionsDocument"));
}

QWidget* SystemOptionsDocument::createPropertiesEditor(QWidget* parent) {
	auto* editor = new QWidget(parent);
	auto* layout = new QVBoxLayout(editor);
	layout->setContentsMargins(0, 0, 0, 0);

	auto* hint = new QLabel(tr("Системные параметры хранятся в глобальном контексте "
		"и применяются сразу."), editor);
	hint->setObjectName(QStringLiteral("EmptyHint"));
	hint->setWordWrap(true);

	auto* properties = new PropertiesListWidget(editor);
	properties->setObjectName(QStringLiteral("SystemOptionsProperties"));
	properties->setScrollable(false);
	properties->setProperties(optionProperties(*this));
	connect(properties, &PropertiesListWidget::propertyChanged, this,
		[this](const QString& name, const QVariant& value) {
			applyOption(name, value);
		});

	auto* reset = new QPushButton(tr("Сбросить к значениям по умолчанию"), editor);
	reset->setObjectName(QStringLiteral("SystemOptionsResetButton"));
	connect(reset, &QPushButton::clicked, this, &SystemOptionsDocument::resetOptions);

	layout->addWidget(hint);
	layout->addWidget(properties, 1);
	layout->addWidget(reset);
	return editor;
}

void SystemOptionsDocument::refresh() {
	// Внешние изменения (консоль, другой документ) подхватываются
	// перестройкой редактора в панели свойств.
	emit propertiesChanged();
}

void SystemOptionsDocument::onContextAttached() {
	ensureOptionVariables();

	// Изменение параметра извне приходит сигналом глобального контекста:
	// в режиме Mutable запись уходит в базовый контекст, поэтому собственный
	// сигнал документа о ней не сообщает.
	const auto context = this->context();
	auto* controller = context ? context->commandController() : nullptr;
	auto global = controller ? controller->globalContext() : nullptr;
	if (!global) {
		return;
	}

	_settingsConnection = connect(global.get(), &CommandContext::variableChanged,
		this, [this](const QString&, const QString& name, const QVariant&,
			const QVariant&) {
			if (_applying || !isOptionName(systemOptionsOf(*this), name)) {
				return;
			}

			emit propertiesChanged();
		});
}

void SystemOptionsDocument::onContextVariableChanged(const QString& name, const QVariant& value) {
	Q_UNUSED(value);
	if (_applying || !isOptionName(systemOptionsOf(*this), name)) {
		return;
	}

	emit propertiesChanged();
}

void SystemOptionsDocument::ensureOptionVariables() {
	auto* registry = systemOptionsOf(*this);
	const auto context = this->context();
	auto* controller = context ? context->commandController() : nullptr;
	if (!registry || !controller) {
		return;
	}

	// Значения по умолчанию лежат в глобальном контексте: без них документ
	// в режиме Mutable записал бы значение в собственные переменные `.options`.
	if (auto global = controller->globalContext()) {
		registry->applyDefaults(*global);
	}
}

void SystemOptionsDocument::applyOption(const QString& name, const QVariant& value) {
	auto* registry = systemOptionsOf(*this);
	const auto option = registry ? registry->option(name) : std::nullopt;
	if (!option) {
		return;
	}

	QVariant converted = value;
	switch (option->type) {
	case SystemOptionType::Boolean:
		converted = value.toBool();
		break;
	case SystemOptionType::Integer:
		converted = value.toInt();
		break;
	case SystemOptionType::Color:
		converted = colorOf(value);
		break;
	case SystemOptionType::Enum:
		converted = optionValueFromTitle(*option, value.toString());
		break;
	case SystemOptionType::Text:
		converted = value.toString();
		break;
	}

	_applying = true;
	const bool applied = setVariable(name, converted);
	_applying = false;

	if (!applied) {
		// Значение не применилось: возвращаем редактор к фактическому значению.
		emit propertiesChanged();
	}
}

void SystemOptionsDocument::resetOptions() {
	auto* registry = systemOptionsOf(*this);
	if (!registry) {
		return;
	}

	_applying = true;
	for (const auto& option : registry->options()) {
		if (!setVariable(option.name, option.defaultValue)) {
			qWarning() << "SystemOptionsDocument: failed to reset option" << option.name;
		}
	}
	_applying = false;

	emit propertiesChanged();
}
