#pragma once
#include <QDialog>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>
#include <optional>

/// Описание свойства: имя и набор mime-типов. Первый mime-тип считается
/// основным — по нему выбирается редактор значения.
struct PropertyDescriptor {
	QString name;
	QStringList mimeTypes;
};

/// Общий диалог имени и набора mime-типов свойства.
///
/// В списке отмечаются текущие mime-типы свойства, служебные пометки
/// (`application/x-mp-shared`, `application/x-mp-context-variable`),
/// дополнительные типы вызывающей стороны и распространённые типы
/// (`PropertyMime::commonMimeTypes()`). Произвольный mime-тип можно добавить
/// вручную — он сразу отмечается. Диалог используется и для свойств узла
/// system_router, и для переменных контекста нового документа.
class PropertyDescriptorDialog : public QDialog {
	Q_OBJECT
public:
	explicit PropertyDescriptorDialog(QWidget* parent = nullptr);
	~PropertyDescriptorDialog() override;

	void setDialogTitle(const QString& title);
	void setName(const QString& name);
	QString name() const;
	/// false — имя только для чтения (правка набора mime-типов).
	void setNameEditable(bool editable);
	void setMimeTypes(const QStringList& mimeTypes);
	QStringList mimeTypes() const;
	/// Дополнительные mime-типы в списке (например, служебные типы
	/// маршрутизатора). Показываются после текущих и до распространённых.
	void setAdditionalMimeTypes(const QStringList& mimeTypes);
	/// Проверка имени: false — кнопка OK недоступна (например, имя уже занято).
	void setNameValidator(std::function<bool(const QString&)> validator);

	/// Показывает диалог; nullopt — отмена или пустое имя.
	/// @param nameEditable false — имя показывается только для чтения
	/// @param additionalMimeTypes дополнительные mime-типы в списке
	/// @param nameValidator необязательная проверка имени
	static std::optional<PropertyDescriptor> request(QWidget* parent, const QString& title,
		const QString& name, const QStringList& mimeTypes, bool nameEditable,
		const QStringList& additionalMimeTypes = QStringList(),
		std::function<bool(const QString&)> nameValidator = {});

private:
	struct Private;
	std::unique_ptr<Private> d;
};
