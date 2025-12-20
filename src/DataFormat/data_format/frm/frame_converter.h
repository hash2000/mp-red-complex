#pragma once
#include "DataFormat/proto/frm.h"
#include "DataFormat/proto/pal.h"

namespace DataFormat::Frm {
void convert(const Proto::Frame &src, const Proto::Pallete &pal,
             Proto::FrameData &dst);
}
