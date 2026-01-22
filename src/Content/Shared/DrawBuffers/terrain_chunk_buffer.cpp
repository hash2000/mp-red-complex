#include "Content/Shared/DrawBuffers/terrain_chunk_buffer.h"
#include "Content/Shared/Draw/draw_program.h"
#include <QOpenGLFunctions>

TerrainChunkDrawBuffer::TerrainChunkDrawBuffer(int widthChunks, int depthChunks)
: _widthChunks(widthChunks)
, _depthChunks(depthChunks)
{
  for (int cz = 0; cz < _depthChunks; cz++) {
		for (int cx = 0; cx < _widthChunks; cx++) {
			getOrCreateChunk(cx, cz);
		}
  }
}

void TerrainChunkDrawBuffer::render(DrawProgram* program, QOpenGLFunctions* fn)
{
	program->setUniformValue("uColor", QVector3D(0.4f, 0.4f, 0.4f));
	fn->glDrawArrays(GL_LINES, 0, _vertexes);
}

TerrainChunk* TerrainChunkDrawBuffer::get(int x, int z) const
{
  auto key = TerrainChunkKey::fromWorld(x, z);
  auto it = _terrain.find(key);
  if (it == _terrain.end()) {
		return nullptr;
  }

  return it->second.get();
}

bool TerrainChunkDrawBuffer::inbox(int x, int z) const
{
  return x >= 0 && x < _widthChunks && z >= 0 && z < _depthChunks;
}

TerrainChunk* TerrainChunkDrawBuffer::getOrCreateChunk(int x, int z)
{
  if (!inbox(x, z)) {
		return nullptr;
  }

  auto key = TerrainChunkKey::fromWorld(x, z);
  auto it = _terrain.find(key);
  if (it == _terrain.end()) {
		auto chunk = std::make_unique<TerrainChunk>(x, z,
			TerrainChunk::CHUNK_SIZE,
			TerrainChunk::CHUNK_SIZE);
		buildChunk(*chunk);
		it = _terrain.emplace(key, std::move(chunk)).first;
  }

  return it->second.get();
}

void TerrainChunkDrawBuffer::buildChunk(TerrainChunk& chunk)
{
  std::vector<float> verts;
  int maxX = std::min(chunk.x() + TerrainChunk::CHUNK_SIZE, chunk.width());
  int maxZ = std::min(chunk.z() + TerrainChunk::CHUNK_SIZE, chunk.depth());

  for (int z = chunk.z(); z < maxZ; ++z)
  {
		for (int x = chunk.x(); x < maxX; ++x)
		{
			float h00 = chunk.height(x, z);
			// Горизонтальные линии
			if (x + 1 < maxX) {
				float h10 = chunk.height(x + 1, z);
				verts.insert(verts.end(), {
						static_cast<float>(x),     h00, static_cast<float>(z),
						static_cast<float>(x + 1), h10, static_cast<float>(z)
					});
			}

			// Вертикальные линии
			if (z + 1 < maxZ) {
				float h01 = chunk.height(x, z + 1);
				verts.insert(verts.end(), {
						static_cast<float>(x), h00, static_cast<float>(z),
						static_cast<float>(x), h01, static_cast<float>(z + 1)
					});
			}
		}
  }

	_vertexes = verts.size();
  create(verts, { {0, 3} });
}
