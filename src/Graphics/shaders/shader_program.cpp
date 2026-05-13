#include "Graphics/shaders/shader_program.h"
#include "Graphics/shaders/uniform_buffer.h"
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLBuffer>

//namespace {
//	// Заполняем данные UBO
//	struct alignas(16) UBOData {
//		QMatrix4x4 projection;
//		QMatrix4x4 view;
//		float zLevel;
//		float _pad1[3];
//		uint flags;
//		float _pad2[3];
//		float time;
//		float tileAnimFrame;
//		QVector2D uvOffset;
//		QVector2D tileSize;
//		QVector4D borderColor;
//		QVector4D wetParams;
//		QVector4D fireParams;
//		float _pad3[4];
//	};
//	constexpr int kUBOSize = sizeof(UBOData);
//	static_assert(kUBOSize == 256, "ShaderProgram UBOData size mismatch");
//}


class ShaderProgram::Private : public QOpenGLFunctions_4_5_Core {
public:
	Private(ShaderProgram* parent) : q(parent) {}
	ShaderProgram* q;

	QOpenGLShaderProgram shader;
	bool initialized = false;
};

ShaderProgram::ShaderProgram()
	: d(std::make_unique<Private>(this)) {
}

ShaderProgram::~ShaderProgram() {
	d->shader.release();
}

bool ShaderProgram::isInitialized() const {
	return d->initialized;
}

bool ShaderProgram::initialize(const QString& vertex, const QString& fragment) {
	if (d->initialized) {
		return true;
	}

	d->initializeOpenGLFunctions();

	if (!d->shader.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex)) {
		qWarning() << "ShaderProgram: failed to compile vertex shader:" << d->shader.log();
		return false;
	}

	if (!d->shader.addShaderFromSourceCode(QOpenGLShader::Fragment, fragment)) {
		qWarning() << "ShaderProgram: failed to compile fragment shader:" << d->shader.log();
		return false;
	}

	if (!d->shader.link()) {
		qWarning() << "ShaderProgram: failed to link shader program:" << d->shader.log();
		return false;
	}

	d->initialized = true;
	return true;
}

void ShaderProgram::bind() {
	d->shader.bind();
}

void ShaderProgram::release() {
	d->shader.release();
}

void ShaderProgram::bindUniformBlock(const char* blockName, uint32_t bindingPoint) {
	if (!d->initialized) {
		return;
	}

	int blockIndex = d->glGetUniformBlockIndex(d->shader.programId(), blockName);
	if (blockIndex == -1) {
		return;
	}

	d->glUniformBlockBinding(d->shader.programId(), blockIndex, bindingPoint);
}

void ShaderProgram::linkBlockToSlot(const char* blockName, uint32_t slot) {
	bindUniformBlock(blockName, slot);
}

bool ShaderProgram::validateBlockSize(const char* blockName, size_t expectedSize) {
	GLuint blockIndex = d->glGetUniformBlockIndex(d->shader.programId(), blockName);
	if (blockIndex == GL_INVALID_INDEX) return false;

	GLint blockSize = 0;
	d->glGetActiveUniformBlockiv(d->shader.programId(), blockIndex,
		GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);

	if (static_cast<size_t>(blockSize) != expectedSize) {
		qWarning() << "UBO size mismatch for" << blockName
			<< ": shader expects" << blockSize
			<< "bytes, buffer has" << expectedSize << "bytes";
		return false;
	}
	return true;
}

uint32_t ShaderProgram::programId() const {
	return d->shader.programId();
}

void ShaderProgram::setUniformValue(const char* name, int value) {
	d->shader.setUniformValue(name, value);
}

void ShaderProgram::setUniformValue(const char* name, float value) {
	d->shader.setUniformValue(name, value);
}

void ShaderProgram::setUniformValue(const char* name, const QMatrix4x4& value) {
	d->shader.setUniformValue(name, value);
}

void ShaderProgram::setUniformValue(const char* name, const QVector4D& value) {
	d->shader.setUniformValue(name, value);
}

//
//void ShaderProgram::setMatrices(const QMatrix4x4& projection, const QMatrix4x4& view) {
//	d->projection = projection;
//	d->view = view;
//}
//
//void ShaderProgram::setTime(float time) {
//	d->currentTime = time;
//}
//
//void ShaderProgram::applyUBO(UniformBuffer& ubo) {
//
//}
//
//void ShaderProgram::updateParams(const MaterialParams& params) {
//	if (!d->initialized) {
//		return;
//	}
//
//	UBOData data;
//	data.projection = d->projection;
//	data.view = d->view;
//	data.zLevel = params.zLevel;
//	data._pad1[0] = data._pad1[1] = data._pad1[2] = 0.0f;
//	data.flags = params.flags;
//	data._pad2[0] = data._pad2[1] = data._pad2[2] = 0.0f;
//	data.time = params.time > 0 ? params.time : d->currentTime;
//	data.tileAnimFrame = params.tileAnimFrame;
//	data.uvOffset = params.uvOffset;
//	data.tileSize = params.tileSize;
//	data.borderColor = QVector4D(
//		params.borderColor.redF(),
//		params.borderColor.greenF(),
//		params.borderColor.blueF(),
//		params.borderColor.alphaF());
//	data.wetParams = params.wetParams;
//	data.fireParams = params.fireParams;
//
//	d->ubo.write(0, &data, sizeof(UBOData));
//}
//
//
