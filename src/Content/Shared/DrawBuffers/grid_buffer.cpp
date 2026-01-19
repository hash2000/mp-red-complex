#include "Content/Shared/DrawBuffers/grid_buffer.h"
#include "Content/Shared/Draw/draw_program.h"
#include <QOpenGLFunctions>

GridDrawBuffer::GridDrawBuffer()
{
	std::vector<float> gridVerts;
	for (int i = 0; i <= 10; i++)
	{
		float f = static_cast<float>(i);
		// Линии вдоль Z (по X)
		gridVerts.push_back(0.0f);
		gridVerts.push_back(0.0f);
		gridVerts.push_back(f);

		gridVerts.push_back(10.0f);
		gridVerts.push_back(0.0f);
		gridVerts.push_back(f);

		gridVerts.push_back(f);
		gridVerts.push_back(0.0f);
		gridVerts.push_back(0.0f);

		gridVerts.push_back(f);
		gridVerts.push_back(0.0f);
		gridVerts.push_back(10.0f);
	}

	_gridVertexCount = gridVerts.size();

	create(gridVerts, { {0, 3} });
}

void GridDrawBuffer::render(DrawProgram* program, QOpenGLFunctions* fn)
{
	program->setUniformValue("uColor", QVector3D(0.4f, 0.4f, 0.4f));
	fn->glDrawArrays(GL_LINES, 0, _gridVertexCount);
}
