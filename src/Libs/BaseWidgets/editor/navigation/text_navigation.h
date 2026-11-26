#pragma once
#include <QList>
#include <QString>
#include <QStringList>

#include <memory>

/// Подставляемая стратегия перемещения курсора по токенам (Ctrl+Left/Right).
///
/// Стратегия работает в пределах одной логической строки: редактор сам
/// переходит на соседнюю строку, когда двигаться дальше некуда. Благодаря
/// этому одну и ту же стратегию используют и редактор кода, и виртуальный
/// редактор больших файлов.
class ITextNavigationStrategy {
public:
	virtual ~ITextNavigationStrategy() = default;

	/// Идентификатор — значение системного параметра `documents.text.navigation`.
	virtual QString id() const = 0;
	/// Подпись для интерфейса настроек.
	virtual QString title() const = 0;
	/// Новая колонка при движении: direction < 0 — влево, > 0 — вправо.
	/// Возвращает ту же колонку, если двигаться некуда.
	virtual int columnAfterMove(const QString& line, int column, int direction) const = 0;
};

/// Базовое поведение: до первого символа другого класса.
///
/// Если первый символ в направлении движения — печатный (буква или цифра),
/// курсор уходит к первому непечатному символу (пробел, табуляция,
/// пунктуация); если первый символ непечатный — к первому печатному.
class WordTextNavigationStrategy final : public ITextNavigationStrategy {
public:
	QString id() const override;
	QString title() const override;
	int columnAfterMove(const QString& line, int column, int direction) const override;

	/// Общий экземпляр для редакторов, которым стратегию не назначили.
	static const WordTextNavigationStrategy& instance();
};

/// Реестр стратегий навигации. Встроенная стратегия регистрируется
/// в конструкторе; новую достаточно зарегистрировать здесь — системный
/// параметр и документы подхватят её сами.
class TextNavigationStrategyRegistry {
public:
	TextNavigationStrategyRegistry();
	~TextNavigationStrategyRegistry();

	void registerStrategy(std::unique_ptr<ITextNavigationStrategy> strategy);
	QList<ITextNavigationStrategy*> strategies() const;
	ITextNavigationStrategy* strategy(const QString& id) const;
	QStringList ids() const;
	QString defaultId() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
