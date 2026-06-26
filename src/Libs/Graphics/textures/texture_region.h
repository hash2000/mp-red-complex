#pragma once

/// UV координаты области в текстурном атласе
struct TextureRegion {
	float u1, v1; // Левый нижний
	float u2, v2; // Правый верхний

	TextureRegion()
		: u1(0), v1(0), u2(0), v2(0) {}

	TextureRegion(float u1, float v1, float u2, float v2)
		: u1(u1), v1(v1), u2(u2), v2(v2) {}
};
