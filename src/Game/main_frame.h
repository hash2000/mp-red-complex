#pragma once
#include "Engine/main_frame.h"
#include "Resources/resources.h"

class GameMainFrame : public MainFrame {
public:
	GameMainFrame(std::shared_ptr<Resources> resources);

	virtual ~GameMainFrame() = default;

private:
	void setupConsole();
	void setupView();

private slots:
	void toggleCommandConsole(bool visible);

private:
	class Private;
	std::unique_ptr<Private> d;
};
