#include "Content/MapEditor/DrawBuffers/axes_buffer.h"
#include "Content/MapEditor/Draw/draw_program.h"
#include <QOpenGLFunctions>

AxesDrawBuffer::AxesDrawBuffer()
{
  std::vector<float> axesVerts = {
		// X axis
		0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Y axis
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Z axis
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f
  };

  _gridVertexCount = axesVerts.size();

  create(axesVerts, { {0, 3} });
}

void AxesDrawBuffer::render(DrawProgram* program, QOpenGLFunctions* fn)
{
  fn->glLineWidth(2.0f);

  // X — красная
  program->setUniformValue("uColor", QVector3D(1.0f, 0.0f, 0.0f));
  fn->glDrawArrays(GL_LINES, 0, 2);

  // Y — зелёная
  program->setUniformValue("uColor", QVector3D(0.0f, 1.0f, 0.0f));
  fn->glDrawArrays(GL_LINES, 2, 2);

  // Z — синяя
  program->setUniformValue("uColor", QVector3D(0.0f, 0.0f, 1.0f));
  fn->glDrawArrays(GL_LINES, 4, 2);

  fn->glLineWidth(1.0f);
}
