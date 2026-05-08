#pragma once
#include <memory>
#include <vector>
#include <functional>

class Material;
class ChunkDrawData;

/// Команда рендеринга - содержит ссылку на данные для отрисовки и материал
struct RenderCommand {
    /// Материал для этой команды
    Material* material = nullptr;
    
    /// Данные для отрисовки (ChunkDrawData)
    ChunkDrawData* drawData = nullptr;
    
    /// Ключ сортировки (вычисляется из материала)
    size_t sortKey = 0;
    
    /// Z-level для сортировки внутри одного материала
    float zLevel = 0.0f;
    
    /// Приоритет (для явного управления порядком)
    int priority = 0;
};

/// Очередь команд рендеринга с сортировкой по состоянию (state sorting)
class RenderQueue {
public:
    RenderQueue();
    ~RenderQueue();

    /// Добавить команду в очередь
    void addCommand(Material* material, ChunkDrawData* drawData, float zLevel = 0.0f, int priority = 0);
    
    /// Очистить очередь
    void clear();
    
    /// Отсортировать команды (group by material, then by zLevel)
    void sort();
    
    /// Выполнить все команды рендеринга
    void execute();
    
    /// Получить количество команд
    size_t commandCount() const;
    
    /// Установить функцию выполнения команды (по умолчанию вызывает render() у drawData)
    using ExecuteFunc = std::function<void(ChunkDrawData*)>;
    void setExecuteFunc(ExecuteFunc func);

private:
    class Private;
    std::unique_ptr<Private> d;
};
