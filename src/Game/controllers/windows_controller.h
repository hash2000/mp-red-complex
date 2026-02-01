#pragma once
#include "Game/mdi_child_window.h"
#include <QObject>
#include <QMdiArea>
#include <memory>

class WindowsController: public QObject {
	Q_OBJECT
public:
	WindowsController();
	~WindowsController() override;

	void setMdiArea(QMdiArea* mdiArea);
	QMdiArea* mdiArea() const;

	/**
	 * Регистрация нового окна в системе
	 * @param window Уникальное окно (владение НЕ передаётся)
	 * @param windowId Уникальный идентификатор (если пустой — генерируется автоматически)
	 * @return Идентификатор окна (может отличаться от запрошенного, если был занят)
	 */
	QString registerWindow(MdiChildWindow* window, const QString& windowId = QString());

	/**
	 * Удаление окна из реестра (вызывается автоматически при уничтожении окна)
	 */
	void unregisterWindow(MdiChildWindow* window);

	/**
	 * Поиск окна по уникальному идентификатору
	 */
	MdiChildWindow* findWindowById(const QString& windowId) const;

	/**
	 * Получение всех зарегистрированных окон
	 */
	QList<MdiChildWindow*> allWindows() const;

	/**
	 * Получение активного окна
	 */
	MdiChildWindow* activeWindow() const;

	/**
	 * Получить все зарегистрированные окна с их идентификаторами
	 * @return Список пар {окно, идентификатор}. Окна проверены на валидность.
	 */
	QList<QPair<QPointer<MdiChildWindow>, QString>> windowEntries() const;
	/**
	 * Получить активное окно с его идентификатором
	 * @return Пара {окно, идентификатор} или {nullptr, ""} если нет активного окна
	 */
	QPair<QPointer<MdiChildWindow>, QString> activeWindowEntry() const;

	/**
	 * Закрыть окно по идентификатору
	 * @return true если окно найдено и закрыто
	 */
	bool closeWindowById(const QString& windowId);

	/**
	 * Закрыть все окна
	 * @return Количество закрытых окон
	 */
	int closeAllWindows();

signals:
	/**
	 * Сигнал о создании нового окна
	 * @param window Указатель на окно
	 * @param windowId Уникальный идентификатор окна
	 */
	void windowCreated(MdiChildWindow* window, const QString& windowId);

	/**
	 * Сигнал о закрытии/удалении окна
	 * @param windowId Идентификатор удалённого окна
	 */
	void windowDestroyed(const QString& windowId);

	/**
	 * Сигнал об изменении активного окна
	 * @param newWindow Новое активное окно (может быть nullptr)
	 * @param oldWindow Предыдущее активное окно (может быть nullptr)
	 */
	void activeWindowChanged(MdiChildWindow* newWindow, MdiChildWindow* oldWindow);

private slots:
	// Внутренние слоты для отслеживания жизненного цикла окон
	void onSubWindowActivated(QMdiSubWindow* window);
	void onSubWindowDestroyed(QObject* obj);

private:
	class Private;
	std::unique_ptr<Private> d;
};
