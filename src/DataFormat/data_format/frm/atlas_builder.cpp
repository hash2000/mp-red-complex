#include "DataFormat/data_format/frm/atlas_builder.h"
#include "DataFormat/data_format/frm/frame_converter.h"
#include <QPainter>

namespace DataFormat::Frm {
void AtlasBuilder::build(const std::vector<Proto::Frame> &src, const Proto::Pallete &pal) {
	const int MAX_ATLAS_WIDTH = 2048;
  int x = 0, y = 0, rowHeight = 0;
  int atlasWidth = 0, atlasHeight = 0;

  _frames.resize(src.size());
	_entries.resize(src.size());

  size_t i = 0;
  for (const auto item : src) {
		auto &f = _frames[i];

    convert(item, pal, f, ConvertOptions());

		int w = f.image.width();
    int h = f.image.height();

    if (x + w > MAX_ATLAS_WIDTH) {
      y += rowHeight;
      x = 0;
      rowHeight = 0;
    }

		_entries[i] = {x, y, w, h, 0, 0, 0, 0, f.offsetX, f.offsetY};

		x += w;
    rowHeight = qMax(rowHeight, h);
    atlasWidth = qMax(atlasWidth, x);
    atlasHeight = qMax(atlasHeight, y + rowHeight);

		i++;
  }

	_atlas = QImage(atlasWidth, atlasHeight, QImage::Format_ARGB32);
	_atlas.fill(Qt::transparent);


  QPainter painter(&_atlas);

  for (int i = 0; i < _frames.size(); ++i) {
    const auto& f = _frames[i];
    const auto& e = _entries[i];

    painter.drawImage(e.x, e.y, f.image);

    // Заполняем UV
    _entries[i].u0 = float(e.x) / atlasWidth;
    _entries[i].v0 = float(e.y) / atlasHeight;
    _entries[i].u1 = float(e.x + e.width) / atlasWidth;
    _entries[i].v1 = float(e.y + e.height) / atlasHeight;
  }

  painter.end();
}


} // namespace DataFormat::Frm
