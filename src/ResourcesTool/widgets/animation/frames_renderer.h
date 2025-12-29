#pragma once
#include "DataFormat/proto/frm.h"
#include "DataFormat/proto/pal.h"
#include <QImage>
#include <vector>

struct IFrameRenderer {
  virtual ~IFrameRenderer() = default;

  virtual void initialize() = 0;
  virtual void uploadFrames(const std::vector<Proto::Frame> &frames, const Proto::Pallete &pallete) = 0;
	virtual void setCurrentFrame(int value) = 0;
  virtual void render() = 0;
  virtual void resize(int width, int height) = 0;
  virtual QString name() const = 0;
  virtual bool initialized() const = 0;
};
