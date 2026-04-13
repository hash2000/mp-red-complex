#include "Graphics/tiles/tile_renderer.h"
#include "Graphics/camera.h"
#include "Graphics/tiles/tileset.h"
#include "Graphics/textures/texture_atlas.h"
#include <QFile>
#include <QOpenGLShaderProgram>
#include <qdebug.h>
#include <unordered_map>
#include <memory>

class TileRenderer::Private {
public:
	Private(TileRenderer* parent)
		: q(parent)
		, shaderProgram(std::make_unique<QOpenGLShaderProgram>()) {
	}

	TileRenderer* q;
	bool initialized = false;
	Tileset* tileset = nullptr;
	float zLevel = 0.0f;
	DebugRenderPassFlags debugRenderPasses = DebugRenderPass::None;

	// Чанки: ключ = (chunkX << 16) | chunkZ
	std::unordered_map<int, std::unique_ptr<Chunk>> chunks;

	// Шейдерная программа
	std::unique_ptr<QOpenGLShaderProgram> shaderProgram;
};

TileRenderer::TileRenderer()
	: d(std::make_unique<Private>(this)) {
}

TileRenderer::~TileRenderer() = default;

bool TileRenderer::initialize() {
	if (d->initialized) {
		return true;
	}

	initializeOpenGLFunctions();

	// Загружаем шейдеры
	d->shaderProgram = std::make_unique<QOpenGLShaderProgram>();

	// Пробуем загрузить из файлов
	QString vertexShaderPath = ":/shaders/tile.vert";
	QString fragmentShaderPath = ":/shaders/tile.frag";

	// Если файлов нет в ресурсах, используем встроенные
	if (!QFile::exists(vertexShaderPath)) {
		d->shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
			#version 330 core
			layout (location = 0) in vec2 aPosition;
			layout (location = 1) in vec2 aTexCoord;
			uniform mat4 uProjection;
			uniform mat4 uView;
			uniform float zLevel;
			out vec2 vTexCoord;
			void main() {
				vTexCoord = aTexCoord;
				gl_Position = uProjection * uView * vec4(aPosition.x, zLevel, aPosition.y, 1.0);
			}
		)");
	} else {
		d->shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderPath);
	}

	if (!QFile::exists(fragmentShaderPath)) {
		d->shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
			#version 330 core
			in vec2 vTexCoord;
			uniform sampler2D uTexture;
			out vec4 FragColor;
			void main() {
				vec4 texColor = texture(uTexture, vTexCoord);
				if (texColor.a < 0.1)
					discard;
				FragColor = texColor;
			}
		)");
	} else {
		d->shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShaderPath);
	}

	if (!d->shaderProgram->link()) {
		qWarning() << "TileRenderer: failed to link shader program:" << d->shaderProgram->log();
		return false;
	}

	d->initialized = true;
	qInfo() << "TileRenderer initialized";
	return true;
}

void TileRenderer::setTileset(Tileset* tileset) {
	d->tileset = tileset;
}

Chunk* TileRenderer::getOrCreateChunk(int chunkX, int chunkZ) {
	int key = (chunkX << 16) | (chunkZ & 0xFFFF);

	auto it = d->chunks.find(key);
	if (it != d->chunks.end()) {
		return it->second.get();
	}

	auto chunk = std::make_unique<Chunk>();
	chunk->initialize();
	chunk->setChunkPosition(chunkX, chunkZ);
	if (d->tileset) {
		chunk->setTileset(d->tileset);
	}

	Chunk* ptr = chunk.get();
	d->chunks[key] = std::move(chunk);
	return ptr;
}

void TileRenderer::removeChunk(int chunkX, int chunkZ) {
	int key = (chunkX << 16) | (chunkZ & 0xFFFF);
	d->chunks.erase(key);
}

void TileRenderer::clearChunks() {
	d->chunks.clear();
}

void TileRenderer::rebuildDirtyChunks() {
	for (auto& [key, chunk] : d->chunks) {
		if (chunk->isDirty()) {
			chunk->rebuild();
		}
	}
}

void TileRenderer::render(const Camera& camera, int viewportWidth, int viewportHeight) {
	if (!d->initialized || !d->shaderProgram) {
		return;
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);

	// Перестраиваем грязные чанки
	rebuildDirtyChunks();

	// Биндим шейдер
	d->shaderProgram->bind();

	// Передаем юниформы
	d->shaderProgram->setUniformValue("uProjection", camera.projection());
	d->shaderProgram->setUniformValue("uView", camera.view());
	d->shaderProgram->setUniformValue("zLevel", d->zLevel);

	if (d->tileset && d->tileset->atlas()) {
		// Биндим текстуру атласа
		d->tileset->atlas()->bind();
		d->shaderProgram->setUniformValue("uTexture", 0); // Привязан к GL_TEXTURE0

		// Рисуем все чанки (frustum culling временно отключен для отладки)
		int visibleCount = 0;
		for (auto& [key, chunk] : d->chunks) {
			if (!chunk->isInitialized()) {
				continue;
			}

			if (isChunkVisible(chunk.get(), camera, viewportWidth, viewportHeight)) {
				chunk->render();
				visibleCount++;
			}
		}

		if (visibleCount > 0) {
			// qDebug() << "TileRenderer: rendered" << visibleCount << "chunks";
		}
	}

	// Отладочные проходы
	renderDebugPasses(camera, viewportWidth, viewportHeight);

	glDisable(GL_BLEND);

	d->shaderProgram->release();
}

bool TileRenderer::isChunkVisible(const Chunk* chunk, const Camera& camera, int viewportWidth, int viewportHeight) const {
	// Проверяем 4 угла чанка в NDC с запасом
	QVector3D corners[4] = {
		QVector3D(chunk->worldMinX(), d->zLevel, chunk->worldMinZ()),
		QVector3D(chunk->worldMaxX(), d->zLevel, chunk->worldMinZ()),
		QVector3D(chunk->worldMinX(), d->zLevel, chunk->worldMaxZ()),
		QVector3D(chunk->worldMaxX(), d->zLevel, chunk->worldMaxZ()),
	};

	QMatrix4x4 mvp = camera.projection() * camera.view();
	
	// Запас на размер чанка (чтобы рисовать чанки, которые частично видны)
	const float margin = 2.0f;

	for (int i = 0; i < 4; ++i) {
		QVector4D clipSpace = mvp * QVector4D(corners[i], 1.0f);
		if (clipSpace.w() > 0) {
			clipSpace /= clipSpace.w();
			// Проверяем X и Y в NDC с запасом
			if (clipSpace.x() >= -1.0f - margin && clipSpace.x() <= 1.0f + margin &&
				clipSpace.y() >= -1.0f - margin && clipSpace.y() <= 1.0f + margin) {
				return true;
			}
		}
	}

	return false;
}

float TileRenderer::zLevel() const {
	return d->zLevel;
}

void TileRenderer::setZLevel(float level) {
	d->zLevel = level;
}

size_t TileRenderer::chunkCount() const {
	return d->chunks.size();
}

void TileRenderer::setDebugRenderPasses(DebugRenderPassFlags passes) {
	d->debugRenderPasses = passes;
}

TileRenderer::DebugRenderPassFlags TileRenderer::debugRenderPasses() const {
	return d->debugRenderPasses;
}

bool TileRenderer::testDebugRenderPass(DebugRenderPass pass) const {
	return d->debugRenderPasses.testFlag(pass);
}

void TileRenderer::renderDebugPasses(const Camera& camera, int viewportWidth, int viewportHeight) {
	if (d->debugRenderPasses == DebugRenderPass::None) {
		return;
	}

	// Отключаем тест глубины для отладочных проходов, чтобы они всегда были поверх
	glDisable(GL_DEPTH_TEST);

	if (testDebugRenderPass(DebugRenderPass::ChunkBorders)) {
		for (auto& [key, chunk] : d->chunks) {
			if (!chunk->isInitialized()) {
				continue;
			}

			chunk->renderBorder();
		}
	}

	glEnable(GL_DEPTH_TEST);
}
