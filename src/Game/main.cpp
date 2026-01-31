#include "Game/main_frame.h"
#include "Engine/application.h"

class GameApplication : public Application {
protected:
	virtual std::unique_ptr<MainFrame> createMainFrame() override {
		return std::make_unique<GameMainFrame>(resources());
	}
};


int main(int argc, char *argv[]) {
	const auto application = std::make_unique<GameApplication>();
	return application->run(argc, argv);
}
