#include "Graphics/tiles/tile_renderer.h"
#include "Graphics/camera.h"
#include "Graphics/tiles/tileset.h"
#include "Graphics/textures/texture_atlas.h"
#include "Graphics/textures/uploaded_texture.h"
#include <QFile>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <qdebug.h>
#include <memory>

class TileRenderer::Private {
public:
	Private(TileRenderer* parent)
		: q(parent)
		, shaderProgram(std::make_unique<QOpenGLShaderProgram>())
		, debugShaderProgram(std::make_unique<QOpenGLShaderProgram>()) {
	}

	TileRenderer* q;
	bool initialized = false;
	Tileset* tileset = nullptr;
	float zLevel = 0.0f;
	QSize chunkSize;
	DebugRenderPassFlags debugRenderPasses = DebugRenderPass::None;

	// Чанки: ключ = (chunkX << 16) | chunkZ
	QHash<QPair<int, int>, QSharedPointer<Chunk>> chunks;

	// Основной шейдер для тайлов
	std::unique_ptr<QOpenGLShaderProgram> shaderProgram;
	
	// Отладочный шейдер (просто цвет, без текстуры)
	std::unique_ptr<QOpenGLShaderProgram> debugShaderProgram;

	QPair<int, int> getChunkKey(int worldX, int worldZ) const {
		int cx = worldX >= 0 ? worldX / chunkSize.width()
			: (worldX - chunkSize.width() + 1) / chunkSize.width();
		int cz = worldZ >= 0 ? worldZ / chunkSize.height()
			: (worldZ - chunkSize.height() + 1) / chunkSize.height();

		QPair<int, int> key{ cx, cz };
		return key;
	}

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

	//#version 330 core
	//out vec4 FragColor;
	//void main() {
	//		FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Ярко-красный
	//}

		//#version 330 core
		//in vec2 vTexCoord;
		//uniform sampler2D uTexture;
		//out vec4 FragColor;
		//void main() {
		//	vec4 texColor = texture(uTexture, vTexCoord);
		//	if (texColor.a < 0.1)
		//		discard;
		//	FragColor = texColor;
		//}
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

	if (!d->shaderProgram->link()) {
		qWarning() << "TileRenderer: failed to link shader program:" << d->shaderProgram->log();
		return false;
	}

	// Инициализируем отладочный шейдер
	d->debugShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
		#version 330 core
		layout (location = 0) in vec2 aPosition;
		uniform mat4 uProjection;
		uniform mat4 uView;
		uniform float zLevel;
		void main() {
			gl_Position = uProjection * uView * vec4(aPosition.x, zLevel, aPosition.y, 1.0);
		}
	)");

	d->debugShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
		#version 330 core
		out vec4 FragColor;
		void main() {
			FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Красная рамка
		}
	)");

	if (!d->debugShaderProgram->link()) {
		qWarning() << "TileRenderer: failed to link debug shader program:" << d->debugShaderProgram->log();
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
	auto key = d->getChunkKey(chunkX, chunkZ);

	auto it = d->chunks.find(key);
	if (it != d->chunks.end()) {
		return it.value().get();
	}

	auto chunk = QSharedPointer<Chunk>::create();
	chunk->setChunkSize(d->chunkSize);
	chunk->setChunkPosition(key.first, key.second);
	if (d->tileset) {
		chunk->setTileset(d->tileset);
	}

	Chunk* ptr = chunk.get();
	d->chunks[key] = chunk;
	return ptr;
}

Chunk* TileRenderer::getChunk(int chunkX, int chunkZ) {
	auto key = d->getChunkKey(chunkX, chunkZ);

	auto it = d->chunks.find(key);
	if (it != d->chunks.end()) {
		return it.value().get();
	}

	return nullptr;
}

void TileRenderer::removeChunk(int chunkX, int chunkZ) {
	auto key = d->getChunkKey(chunkX, chunkZ);
	d->chunks.remove(key);
}

void TileRenderer::clearChunks() {
	d->chunks.clear();
}

void TileRenderer::rebuildDirtyChunks() {
	for (auto& chunk : d->chunks) {
		if (chunk->isDirty()) {
			chunk->rebuild();
		}
	}
}

void TileRenderer::render(const Camera& camera, int viewportWidth, int viewportHeight) {
	if (!d->initialized || !d->shaderProgram) {
		return;
	}

	// Перестраиваем грязные чанки
	rebuildDirtyChunks();

	// Рисуем чанки только если есть tileset
	if (d->tileset && d->tileset->atlas() && d->tileset->atlas()->isLoaded()) {

		auto atlas = d->tileset->atlas();
		auto f = QOpenGLContext::currentContext()->functions();

		// Отладочный вывод (только первый кадр после изменений)
		static int frameCount = 0;
		if (frameCount++ < 3) {
			qInfo() << "TileRenderer::render - tileset atlas:" << d->tileset->atlas()
				<< "textureId:" << d->tileset->atlas()->texture()->textureId()
				<< "chunks:" << d->chunks.size();
		}

		f->glEnable(GL_BLEND);
		f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		f->glEnable(GL_DEPTH_TEST);

		// Биндим шейдер
		d->shaderProgram->bind();

		// Передаем юниформы
		d->shaderProgram->setUniformValue("uProjection", camera.projection());
		d->shaderProgram->setUniformValue("uView", camera.view());
		d->shaderProgram->setUniformValue("zLevel", d->zLevel);

		// Биндим текстуру атласа
		f->glActiveTexture(GL_TEXTURE0);
		atlas->bind();

		d->shaderProgram->setUniformValue("uTexture", 0); // Привязан к GL_TEXTURE0

		// Рисуем все чанки (frustum culling временно отключен для отладки)
		int visibleCount = 0;
		for (const auto& chunk : d->chunks) {
			const auto ptr = chunk.get();
			if (!ptr->isInitialized()) {
				continue;
			}

			if (isChunkVisible(ptr, camera, viewportWidth, viewportHeight)) {
				ptr->render();
				visibleCount++;
			}
		}

		if (frameCount <= 3) {
			qInfo() << "TileRenderer::render - rendered" << visibleCount << "chunks";
		}

		glDisable(GL_BLEND);

		atlas->unbind();
		d->shaderProgram->release();
	}

	// Отладочные проходы (рисуются всегда, даже без tileset)
	renderDebugPasses(camera, viewportWidth, viewportHeight);
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
		auto w = clipSpace.w();
		if (w > 0) {
			clipSpace /= w;
			// Проверяем X и Y в NDC с запасом
			auto x = clipSpace.x();
			auto y = clipSpace.y();
			if (
				x >= -1.0f - margin && x <= 1.0f + margin &&
				y >= -1.0f - margin && y <= 1.0f + margin) {
				return true;
			}
		}
	}

	return false;
}

void TileRenderer::setChunkSize(const QSize& chunkSize) {
	d->chunkSize = chunkSize;
}

QSize TileRenderer::chunkSize() const {
	return d->chunkSize;
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

	// Биндим отладочный шейдер
	d->debugShaderProgram->bind();
	d->debugShaderProgram->setUniformValue("uProjection", camera.projection());
	d->debugShaderProgram->setUniformValue("uView", camera.view());
	d->debugShaderProgram->setUniformValue("zLevel", d->zLevel);

	// Отключаем тест глубины для отладочных проходов, чтобы они всегда были поверх
	glDisable(GL_DEPTH_TEST);

	if (testDebugRenderPass(DebugRenderPass::ChunkBorders)) {
		for (auto& chunk : d->chunks) {
			if (!chunk->isInitialized()) {
				continue;
			}

			chunk->renderBorder();
		}
	}

	d->debugShaderProgram->release();
	glEnable(GL_DEPTH_TEST);
}
