#version 330 core

// Вершинный шейдер для тайлов
layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uProjection;
uniform mat4 uView;
uniform float zLevel; // Уровень высоты (Y)

out vec2 vTexCoord;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * vec4(aPosition.x, zLevel, aPosition.y, 1.0);
}
