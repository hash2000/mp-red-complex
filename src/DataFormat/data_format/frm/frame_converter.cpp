#include "DataFormat/data_format/frm/frame_converter.h"

namespace DataFormat::Frm {

void convert(const Proto::Frame &src, const Proto::Pallete &pal,
	Proto::FrameData &dst, const ConvertOptions& options) {
  dst.width = src.width;
  dst.height = src.height;
  dst.offsetX = src.offsetX;
  dst.offsetY = src.offsetY;

  dst.image = QImage(dst.width, dst.height, QImage::Format_ARGB32);
  dst.image.fill(Qt::transparent);

  if (pal.items.size() == 0) {
    return;
  }

  auto *bits = reinterpret_cast<QRgb *>(dst.image.bits());
  size_t i = 0;
  for (int y = 0; y < dst.height && i < src.indexes.size(); y++) {
    for (int x = 0; x < dst.width && i < src.indexes.size(); x++, i++) {
      const auto idx = std::to_integer<uint8_t>(src.indexes[i]);
      const auto color = pal.items[idx];

			const auto r = qRed(color);
			const auto g = qGreen(color);
			const auto b = qBlue(color);
			int a = 255;

			if (options.useAlphaColor && options.alphaColor == color) {
				a = options.alpha;
			}

      bits[y * dst.width + x] = qRgba(r, g, b, a);
    }
  }
}
} // namespace DataFormat::Frm
