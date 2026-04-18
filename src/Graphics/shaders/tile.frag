#version 330 core

// Фрагментный шейдер для тайлов
in vec2 vTexCoord;

uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    if (texColor.a < 0.1)
        discard; // Отбрасываем полностью прозрачные пиксели
    FragColor = texColor;
}
