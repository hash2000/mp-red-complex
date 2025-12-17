#include "Game/data_format/frm/data_reader.h"
#include <QDebug>

namespace DataFormat::Frm {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
	}

	void DataReader::read(Proto::Animation &result, const QString &suffix) {

		uint8_t directionId = 0xFF;

		if (suffix.length() == 3 && suffix.startsWith("fr")) {
			bool ok;
			const auto ch = suffix.mid(2);
			const auto selected = ch.toInt(&ok);
			if (ok) {
				directionId = selected;
			}
		}

		result.version = _stream.u32();
		result.fps = _stream.u16();
		result.current = _stream.u16();
		result.current = result.current == 0xffff ? 0 : result.current;
		result.fpd = _stream.u16();
		result.fpd = result.fpd == 0xffff ? 0 : result.fpd;
		// result.xShift = _stream.u16();
		// result.xShift = result.xShift == 0xffff ? 0 : result.xShift;
		// _stream.skip(2);

		readDirectionsOffsets(result.directions);
		readFrames(result);
	}

	void DataReader::readDirectionsOffsets(std::vector<Proto::Direction> &directions) {
		uint16_t x[6];
		uint16_t y[6];

		for (uint8_t i = 0; i < 6; i++) {
			x[i] = _stream.u16();
			x[i] = x[i] == 0xffff ? 0 : x[i];
		}

		for (uint8_t i = 0; i < 6; i++) {
			y[i] = _stream.u16();
			y[i] = y[i] == 0xffff ? 0 : y[i];
		}

		uint32_t prevOffset = 0xffffffff;
		for (uint8_t i = 0; i < 6; i++) {
			const auto offset = _stream.u32();
			if (prevOffset == offset) {
				continue;
			}

			prevOffset = offset;
			auto &it = directions.emplace_back();
			it.dataOffset = offset;
			it.shiftX = x[i];
			it.shiftY = y[i];
		}

		_stream.skip(4);
		_bodyPos = _stream.position();
		if (_bodyPos != 0x003E) {
			throw std::runtime_error(QString("Load frames [%1], position [%2]. Invalid body pos %3 expected %4.")
				.arg(_stream.name())
				.arg(_stream.position())
				.arg(_bodyPos)
				.arg(0x003E)
				.toStdString());
		}
	}

	void DataReader::readFrames(Proto::Animation &result) {
		for (auto &direction : result.directions) {
			_stream.position(_bodyPos + direction.dataOffset);
			direction.frames.resize(result.fpd);

			for (auto &frame: direction.frames) {

				frame.width = _stream.u16();
				frame.height = _stream.u16();
				frame.frameSize = _stream.u32();
				frame.offsetX = _stream.i16();
				frame.offsetY = _stream.i16();

				const auto frameSize = frame.width * frame.height;
				if (frameSize != frame.frameSize) {
					throw std::runtime_error(QString("Load frames [%1], position [%2]. Invalid frame size %3 expected %4.")
						.arg(_stream.name())
						.arg(_stream.position())
						.arg(frame.frameSize)
						.arg(frameSize)
						.toStdString());
				}

				frame.indexes.resize(frame.frameSize);

				_stream.read(frame.indexes);
			}
		}
	}
}
