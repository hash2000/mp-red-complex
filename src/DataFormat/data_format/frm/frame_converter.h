#pragma once
#include "DataFormat/proto/frm.h"
#include "DataFormat/proto/pal.h"

namespace DataFormat::Frm {
	struct ConvertOptions {
		bool useAlphaColor = true;
		QRgb alphaColor = qRgb(255, 255, 255);
		int alpha = 0;
	};

	void convert(const Proto::Frame &src, const Proto::Pallete &pal,
		 Proto::FrameData &dst, const ConvertOptions &options);
}
