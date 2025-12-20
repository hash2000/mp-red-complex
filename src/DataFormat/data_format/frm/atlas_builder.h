#pragma once
#include "DataFormat/proto/frm.h"
#include "DataFormat/proto/pal.h"
#include "Base/container_view.h"
#include <QImage>
#include <vector>

namespace DataFormat::Frm {
class AtlasBuilder {
public:
  void build(const std::vector<Proto::Frame> &src, const Proto::Pallete &pal);

	auto frames() const {
    return make_deref_view(_frames);
	}

	auto entries() const {
		return make_deref_view(_entries);
	}

	const QImage& atlas() const {
		return _atlas;
	}

private:
  std::vector<Proto::FrameData> _frames;
  std::vector<Proto::AtlasEntry> _entries;
  QImage _atlas;
};

} // namespace DataFormat::Frm
