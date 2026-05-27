#include "BaseWidgets/highlights/rules/highlighter_rule_glsl.h"
#include "BaseWidgets/highlights/highlighter.h"

namespace highlighter::rule::glsl {

void apply(Highlighter& highlighter) {
	// ==========================================
	// 1. Ключевые слова GLSL (управляющие конструкции)
	// ==========================================
	QTextCharFormat keywordFormat;
	keywordFormat.setForeground(QColor("#569CD6"));  // синий, как в VS Code
	keywordFormat.setFontWeight(QFont::Bold);

	highlighter.addRule("glsl_keywords",
		"\\b(if|else|for|while|do|switch|case|default|break|continue|return|discard)\\b",
		keywordFormat, 10);  // высокий приоритет

	// ==========================================
	// 2. Квалификаторы переменных
	// ==========================================
	QTextCharFormat qualifierFormat;
	qualifierFormat.setForeground(QColor("#4EC9B0"));  // бирюзовый
	qualifierFormat.setFontWeight(QFont::Bold);

	highlighter.addRule("glsl_qualifiers",
		"\\b(uniform|in|out|inout|attribute|varying|const|flat|smooth|noperspective|"
		"centroid|sample|patch|invariant|precise|highp|mediump|lowp|readonly|writeonly|"
		"coherent|volatile|restrict|layout|shared|packed|std140|std430)\\b",
		qualifierFormat, 11);

	// ==========================================
	// 3. Типы данных
	// ==========================================
	QTextCharFormat typeFormat;
	typeFormat.setForeground(QColor("#4EC9B0"));  // бирюзовый
	typeFormat.setFontWeight(QFont::Normal);

	highlighter.addRule("glsl_types",
		"\\b(void|bool|int|uint|float|double|vec2|vec3|vec4|dvec2|dvec3|dvec4|"
		"bvec2|bvec3|bvec4|ivec2|ivec3|ivec4|uvec2|uvec3|uvec4|"
		"mat2|mat3|mat4|mat2x2|mat2x3|mat2x4|mat3x2|mat3x3|mat3x4|"
		"mat4x2|mat4x3|mat4x4|dmat2|dmat3|dmat4|dmat2x2|dmat2x3|dmat2x4|"
		"dmat3x2|dmat3x3|dmat3x4|dmat4x2|dmat4x3|dmat4x4|"
		"sampler1D|sampler2D|sampler3D|samplerCube|sampler1DShadow|"
		"sampler2DShadow|samplerCubeShadow|sampler1DArray|sampler2DArray|"
		"sampler1DArrayShadow|sampler2DArrayShadow|sampler2DMS|sampler2DMSArray|"
		"samplerBuffer|sampler2DRect|sampler2DRectShadow|isampler1D|isampler2D|"
		"isampler3D|isamplerCube|isampler1DArray|isampler2DArray|isamplerBuffer|"
		"isampler2DRect|usampler1D|usampler2D|usampler3D|usamplerCube|"
		"usampler1DArray|usampler2DArray|usamplerBuffer|usampler2DRect|"
		"samplerCubeArray|samplerCubeArrayShadow|isamplerCubeArray|usamplerCubeArray|"
		"image1D|image2D|image3D|imageCube|image1DArray|image2DArray|imageBuffer|"
		"image2DRect|iimage1D|iimage2D|iimage3D|iimageCube|iimage1DArray|"
		"iimage2DArray|iimageBuffer|iimage2DRect|uimage1D|uimage2D|uimage3D|"
		"uimageCube|uimage1DArray|uimage2DArray|uimageBuffer|uimage2DRect|"
		"atomic_uint|struct)\\b",
		typeFormat, 12);

	// ==========================================
	// 4. Встроенные функции
	// ==========================================
	QTextCharFormat functionFormat;
	functionFormat.setForeground(QColor("#DCDCAA"));  // желтоватый
	functionFormat.setFontWeight(QFont::Normal);

	highlighter.addRule("glsl_builtin_functions",
		"\\b(radians|degrees|sin|cos|tan|asin|acos|atan|sinh|cosh|tanh|"
		"asinh|acosh|atanh|pow|exp|log|exp2|log2|sqrt|inversesqrt|"
		"abs|sign|floor|trunc|round|roundEven|ceil|fract|mod|modf|"
		"min|max|clamp|mix|step|smoothstep|isnan|isinf|"
		"floatBitsToInt|floatBitsToUint|intBitsToFloat|uintBitsToFloat|"
		"fma|frexp|ldexp|"
		"length|distance|dot|cross|normalize|faceforward|reflect|refract|"
		"matrixCompMult|outerProduct|transpose|determinant|inverse|"
		"lessThan|lessThanEqual|greaterThan|greaterThanEqual|equal|notEqual|"
		"any|all|not|"
		"packUnorm2x16|packUnorm4x8|packSnorm4x8|unpackUnorm2x16|unpackUnorm4x8|unpackSnorm4x8|"
		"packDouble2x32|unpackDouble2x32|packHalf2x16|unpackHalf2x16|"
		"textureSize|textureQueryLod|textureQueryLevels|texture|textureProj|textureLod|"
		"textureOffset|texelFetch|texelFetchOffset|textureProjOffset|textureLodOffset|"
		"textureProjLod|textureProjLodOffset|textureGrad|textureGradOffset|"
		"textureProjGrad|textureProjGradOffset|textureGather|textureGatherOffset|"
		"textureGatherOffsets|"
		"atomicCounterIncrement|atomicCounterDecrement|atomicCounter|"
		"atomicAdd|atomicMin|atomicMax|atomicAnd|atomicOr|atomicXor|"
		"atomicExchange|atomicCompSwap|"
		"imageLoad|imageStore|imageAtomicAdd|imageAtomicMin|imageAtomicMax|"
		"imageAtomicAnd|imageAtomicOr|imageAtomicXor|imageAtomicExchange|imageAtomicCompSwap|"
		"memoryBarrier|memoryBarrierAtomicCounter|memoryBarrierBuffer|"
		"memoryBarrierShared|memoryBarrierImage|groupMemoryBarrier|barrier|"
		"EmitVertex|EndPrimitive)\\b",
		functionFormat, 20);

	// ==========================================
	// 5. Встроенные константы
	// ==========================================
	QTextCharFormat constantFormat;
	constantFormat.setForeground(QColor("#569CD6"));  // синий
	constantFormat.setFontWeight(QFont::Normal);

	highlighter.addRule("glsl_constants",
		"\\b(gl_Position|gl_PointSize|gl_ClipDistance|gl_CullDistance|"
		"gl_FragCoord|gl_FrontFacing|gl_FragDepth|gl_FragColor|gl_FragData|"
		"gl_VertexID|gl_InstanceID|gl_DrawID|gl_BaseVertex|gl_BaseInstance|"
		"gl_PrimitiveID|gl_InvocationID|gl_Layer|gl_ViewportIndex|"
		"gl_TessLevelOuter|gl_TessLevelInner|gl_TessCoord|gl_PatchVerticesIn|"
		"gl_GlobalInvocationID|gl_LocalInvocationID|gl_WorkGroupSize|gl_WorkGroupID|"
		"gl_NumWorkGroups|gl_MaxVertexAttribs|gl_MaxCombinedTextureImageUnits|"
		"gl_MaxDrawBuffers|gl_MaxClipDistances|gl_MaxTextureImageUnits|"
		"true|false|PI|TAU|E)\\b",
		constantFormat, 15);

	// ==========================================
	// 6. Комментарии
	// ==========================================
	QTextCharFormat commentFormat;
	commentFormat.setForeground(QColor("#6A9955"));  // зеленый
	commentFormat.setFontItalic(true);

	highlighter.addRule("glsl_single_line_comments",
		"//[^\n]*",
		commentFormat, 5);

	highlighter.addRule("glsl_multi_line_comments",
		"/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/",
		commentFormat, 5);

	// ==========================================
	// 7. Строки
	// ==========================================
	QTextCharFormat stringFormat;
	stringFormat.setForeground(QColor("#CE9178"));  // оранжевый
	stringFormat.setFontWeight(QFont::Normal);

	highlighter.addRule("glsl_strings",
		"\"[^\"]*\"",
		stringFormat, 6);

	// ==========================================
	// 8. Числа
	// ==========================================
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));  // светло-зеленый

	highlighter.addRule("glsl_numbers",
		"\\b[0-9]+\\.?[0-9]*(?:[eE][+-]?[0-9]+)?[fF]?\\b|"
		"\\b0[xX][0-9a-fA-F]+\\b|"
		"\\b0[0-7]*\\b",
		numberFormat, 7);

	// ==========================================
	// 9. Препроцессор
	// ==========================================
	QTextCharFormat preprocessorFormat;
	preprocessorFormat.setForeground(QColor("#9B9B9B"));  // серый
	preprocessorFormat.setFontWeight(QFont::Bold);

	highlighter.addRule("glsl_preprocessor",
		"^\\s*#\\s*(version|extension|define|undef|if|ifdef|ifndef|else|elif|endif|error|pragma|include|line)\\b",
		preprocessorFormat, 3);

	// ==========================================
	// 10. Специальные символы и операторы (опционально)
	// ==========================================
	QTextCharFormat operatorFormat;
	operatorFormat.setForeground(QColor("#D4D4D4"));  // светло-серый

	highlighter.addRule("glsl_operators",
		"[\\+\\-\\*\\/%=<>!&|^~?:]+|\\b(and|or|xor|not|mod)\\b",
		operatorFormat, 30);
}

void remove(Highlighter& highlighter) {
	highlighter.deleteRule("glsl_keywords");
	highlighter.deleteRule("glsl_qualifiers");
	highlighter.deleteRule("glsl_types");
	highlighter.deleteRule("glsl_builtin_functions");
	highlighter.deleteRule("glsl_constants");
	highlighter.deleteRule("glsl_single_line_comments");
	highlighter.deleteRule("glsl_multi_line_comments");
	highlighter.deleteRule("glsl_strings");
	highlighter.deleteRule("glsl_numbers");
	highlighter.deleteRule("glsl_preprocessor");
	highlighter.deleteRule("glsl_operators");
}

}
