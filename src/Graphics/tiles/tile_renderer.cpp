#include "Graphics/tiles/tile_renderer.h"
#include "Graphics/camera.h"
#include "Graphics/tiles/material.h"
#include "Graphics/tiles/tileset_material.h"
#include "Graphics/tiles/border_material.h"
#include "Graphics/tiles/chunk/chunk_draw_tileset.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <qdebug.h>
#include <memory>

class TileRenderer::Private {
public:
Private(TileRenderer* parent)
: q(parent)
, tilesetMaterial(std::make_unique<TilesetMaterial>())
, borderMaterial(std::make_unique<BorderMaterial>()) {
}

TileRenderer* q;
bool initialized = false;
QSize chunkSize;
DebugRenderPassFlags debugRenderPasses = DebugRenderPass::None;

// Чанки: ключ = (chunkX << 16) | chunkZ
QHash<QPair<int, int>, QSharedPointer<Chunk>> chunks;

// Основной материал для тайлсетов
std::unique_ptr<TilesetMaterial> tilesetMaterial;

// Материал для отладочных рамок
std::unique_ptr<BorderMaterial> borderMaterial;

// Очередь рендеринга
RenderQueue renderQueue;

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

// Инициализируем материалы
if (!d->tilesetMaterial->initialize()) {
qWarning() << "TileRenderer: failed to initialize tileset material";
return false;
}

if (!d->borderMaterial->initialize()) {
qWarning() << "TileRenderer: failed to initialize border material";
return false;
}

d->initialized = true;
qInfo() << "TileRenderer initialized with new material system";
return true;
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

// Добавляем слой с тайлсетом и материалом по умолчанию
auto tilesetDrawData = std::make_unique<ChunkDrawTileset>();
tilesetDrawData->setChunkSize(d->chunkSize);
chunk->addLayer(std::move(tilesetDrawData), d->tilesetMaterial.get());

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
chunk->rebuild();
}
}

void TileRenderer::showOnlyOneLayer(int layer) {
for (auto& chunk : d->chunks) {
chunk->showOnlyOneLayer(layer);
}
}

void TileRenderer::showAllLayers() {
for (auto& chunk : d->chunks) {
chunk->showAllLayers();
}
}

void TileRenderer::render(const Camera& camera, int viewportWidth, int viewportHeight) {
if (!d->initialized) {
return;
}

// Перестраиваем грязные чанки
rebuildDirtyChunks();

renderTileSet(camera, viewportWidth, viewportHeight);

renderDebugPasses(camera, viewportWidth, viewportHeight);
}

void TileRenderer::renderTileSet(const Camera& camera, int viewportWidth, int viewportHeight) {
auto f = QOpenGLContext::currentContext()->functions();

f->glDisable(GL_DEPTH_TEST);
f->glDepthMask(GL_TRUE);
f->glEnable(GL_BLEND);
f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// Очищаем и заполняем очередь рендеринга
d->renderQueue.clear();

// Устанавливаем матрицы в материалы
const auto projection = camera.projection();
const auto view = camera.view();

if (d->tilesetMaterial->shaderProgram) {
d->tilesetMaterial->shaderProgram->bind();
d->tilesetMaterial->shaderProgram->setUniformValue("uProjection", projection);
d->tilesetMaterial->shaderProgram->setUniformValue("uView", view);
d->tilesetMaterial->shaderProgram->release();
}

if (d->borderMaterial->shaderProgram) {
d->borderMaterial->shaderProgram->bind();
d->borderMaterial->shaderProgram->setUniformValue("uProjection", projection);
d->borderMaterial->shaderProgram->setUniformValue("uView", view);
d->borderMaterial->shaderProgram->release();
}

// Собираем команды рендеринга из всех чанков
for (const auto& chunk : d->chunks) {
const auto ptr = chunk.get();

if (isChunkVisible(ptr, camera, viewportWidth, viewportHeight)) {
ptr->render(d->renderQueue);
}
}

// Сортируем и выполняем команды
d->renderQueue.sort();
d->renderQueue.execute();
}

bool TileRenderer::isChunkVisible(const Chunk* chunk, const Camera& camera, int viewportWidth, int viewportHeight) const {
// Проверяем 4 угла чанка в NDC с запасом
QVector3D corners[4] = {
QVector3D(chunk->worldMinX(), chunk->zLevel(), chunk->worldMinZ()),
QVector3D(chunk->worldMaxX(), chunk->zLevel(), chunk->worldMinZ()),
QVector3D(chunk->worldMinX(), chunk->zLevel(), chunk->worldMaxZ()),
QVector3D(chunk->worldMaxX(), chunk->zLevel(), chunk->worldMaxZ()),
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

// Устанавливаем матрицы в border материал
const auto projection = camera.projection();
const auto view = camera.view();

if (d->borderMaterial->shaderProgram) {
d->borderMaterial->shaderProgram->bind();
d->borderMaterial->shaderProgram->setUniformValue("uProjection", projection);
d->borderMaterial->shaderProgram->setUniformValue("uView", view);
d->borderMaterial->shaderProgram->release();
}

// Отключаем тест глубины для отладочных проходов, чтобы они всегда были поверх
glDisable(GL_DEPTH_TEST);

QColor selectedBorderColor;
if (testDebugRenderPass(DebugRenderPass::ChunkBorders)) {
// Очищаем очередь и собираем только border команды
d->renderQueue.clear();

for (auto& chunk : d->chunks) {
const auto borderColor = chunk->borderColor();
d->borderMaterial->setColor(borderColor);
d->borderMaterial->setZLevel(chunk->zLevel());

chunk->renderBorder(d->renderQueue);
}

d->renderQueue.sort();
d->renderQueue.execute();
}

glEnable(GL_DEPTH_TEST);
}

TilesetMaterial* TileRenderer::tilesetMaterial() const {
return d->tilesetMaterial.get();
}

BorderMaterial* TileRenderer::borderMaterial() const {
return d->borderMaterial.get();
}
