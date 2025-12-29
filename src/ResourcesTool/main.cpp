#include "Engine/application.h"
#include "ResourcesTool/main_frame/resources_viewer/resources_viewer_frame.h"

class ResourcesToolApplication : public Application {
protected:
	virtual std::unique_ptr<MainFrame> createMainFrame() override {
		return std::make_unique<ResourcesViewerFrame>(resources());
	}
};


int main(int argc, char *argv[]) {
	const auto application = std::make_unique<ResourcesToolApplication>();
	return application->run(argc, argv);
}
