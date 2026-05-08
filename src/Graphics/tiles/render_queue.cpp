#include "Graphics/tiles/render_queue.h"
#include "Graphics/tiles/material.h"
#include "Graphics/tiles/chunk/chunk_draw_data.h"
#include <algorithm>

class RenderQueue::Private {
public:
    Private(RenderQueue* parent) : q(parent) {}

    RenderQueue* q;
    std::vector<RenderCommand> commands;
    ExecuteFunc executeFunc;

    void defaultExecute(ChunkDrawData* drawData) {
        if (drawData) {
            drawData->render();
        }
    }
};

RenderQueue::RenderQueue() 
    : d(std::make_unique<Private>(this)) {
    d->executeFunc = [this](ChunkDrawData* data) { d->defaultExecute(data); };
}

RenderQueue::~RenderQueue() = default;

void RenderQueue::addCommand(Material* material, ChunkDrawData* drawData, float zLevel, int priority) {
    RenderCommand cmd;
    cmd.material = material;
    cmd.drawData = drawData;
    cmd.zLevel = zLevel;
    cmd.priority = priority;
    
    if (material) {
        cmd.sortKey = material->sortKey();
    } else {
        cmd.sortKey = 0;
    }
    
    d->commands.push_back(cmd);
}

void RenderQueue::clear() {
    d->commands.clear();
}

void RenderQueue::sort() {
    // Сортировка: сначала по ключу материала (group by material), затем по zLevel, затем по приоритету
    std::sort(d->commands.begin(), d->commands.end(), 
        [](const RenderCommand& a, const RenderCommand& b) {
            // Сначала сортируем по ключу материала (группируем одинаковые материалы)
            if (a.sortKey != b.sortKey) {
                return a.sortKey < b.sortKey;
            }
            
            // Затем по zLevel (для правильного порядка прозрачности)
            if (a.zLevel != b.zLevel) {
                return a.zLevel < b.zLevel;
            }
            
            // Затем по приоритету
            if (a.priority != b.priority) {
                return a.priority < b.priority;
            }
            
            return false;
        });
}

void RenderQueue::execute() {
    if (d->commands.empty()) {
        return;
    }

    Material* currentMaterial = nullptr;
    
    for (const auto& cmd : d->commands) {
        // Если материал изменился - отвязываем старый и привязываем новый
        if (cmd.material != currentMaterial) {
            if (currentMaterial) {
                currentMaterial->unbind();
            }
            
            if (cmd.material) {
                cmd.material->setZLevel(cmd.zLevel);
                cmd.material->bind();
            }
            
            currentMaterial = cmd.material;
        }
        
        // Выполняем команду рендеринга
        if (cmd.drawData && d->executeFunc) {
            d->executeFunc(cmd.drawData);
        }
    }
    
    // Отвязываем последний материал
    if (currentMaterial) {
        currentMaterial->unbind();
    }
}

size_t RenderQueue::commandCount() const {
    return d->commands.size();
}

void RenderQueue::setExecuteFunc(ExecuteFunc func) {
    d->executeFunc = func;
}
