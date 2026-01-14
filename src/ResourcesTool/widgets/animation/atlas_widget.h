#pragma once
#include "ResourcesTool/widgets/base_tab_widget.h"
#include "DataFormat/proto/frm.h"
#include "DataFormat/proto/pal.h"
#include <memory>

class AtlasWidget : public BaseTabWidget {
public:
	AtlasWidget(
		std::shared_ptr<const Proto::Animation> animation,
		std::shared_ptr<const Proto::Pallete> pallete, QWidget* parent = nullptr);

};
