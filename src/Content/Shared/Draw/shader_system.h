#pragma once
#include "Content/Shared/Draw/draw_program.h"
#include "BaseWidgets/tabs/base_tab_widget.h"
#include <list>
#include <memory>

class ShaderSystem {
public:
	ShaderSystem(std::shared_ptr<Resources> resources, const QVariantMap& params);

private:
	std::list<std::unique_ptr<DrawProgram>> _shaders;
};
