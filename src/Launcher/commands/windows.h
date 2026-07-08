//#pragma once
//#include "Content/ConsoleModule/i_command.h"
//#include <memory>
//
//class WindowsCommand : public ICommand {
//	Q_OBJECT
//public:
//	WindowsCommand(QObject* parent = nullptr);
//	~WindowsCommand() override;
//
//	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;
//
//	QString name() const override {
//		return "windows";
//	}
//	QString description() const override {
//		return "Operations with application windows";
//	}
//	QString help() const override;
//
//private:
//	class Private;
//	std::unique_ptr<Private> d;
//};
