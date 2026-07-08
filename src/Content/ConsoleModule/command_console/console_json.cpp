#include "Content/ConsoleModule/command_console/console_json.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QVector>

namespace {
struct JsonNode {
	QString key;
	QString value;
	QString type;
	QVector<JsonNode> children;
	int depth = 0;
};
struct JsonStyleConfig {
	QString keyColor = "#9cdcfe";
	QString stringColor = "#ce9178";
	QString numberColor = "#b5cea8";
	QString booleanColor = "#569cd6";
	QString nullColor = "#808080";
	QString bracketColor = "#d4d4d4";
	QString punctuationColor = "#d4d4d4";
	QString commentColor = "#6a9955";

	QString backgroundColor = "#1e1e1e";
	QString indentSize = "20px";
	QString fontSize = "10pt";
	QString fontFamily = "Consolas, 'Courier New', monospace";

	bool collapseLongArrays = true;
	int maxArrayItems = 10;
	int maxExpandLevel = 3;
};
}

class ConsoleJson::Private {
public:
	Private(ConsoleJson* parent) : q(parent) {}
	ConsoleJson* q;

	QJsonDocument jsonDoc;
	JsonNode rootNode;
	bool isValid = false;
	QString errorMessage;
	JsonStyleConfig style;

	void setStyleConfig(const JsonStyleConfig& config);
	JsonNode buildNode(const QJsonDocument& doc, const QString& key = "", int depth = 0) const;
	QString renderNode(const JsonNode& node, int depth = 0, bool isLast = true) const;
	QString renderValue(const QJsonValue& value, int depth = 0, const QString& key = "") const;
	JsonNode buildNode(const QJsonValue& value, const QString& key = "", int depth = 0) const;
	QString escapeJsonString(const QString& str) const;
	QString getIndent(int depth) const;
	QString getValueType(const QJsonValue& value) const;
};

ConsoleJson::ConsoleJson()
	: d(std::make_unique<Private>(this)) {
}

ConsoleJson::ConsoleJson(const QString& jsonString)
	: d(std::make_unique<Private>(this)) {
	load(jsonString);
}

ConsoleJson::ConsoleJson(const QJsonDocument& jsonDoc)
	: d(std::make_unique<Private>(this)) {
	load(jsonDoc);
}

ConsoleJson::ConsoleJson(const QByteArray& jsonData)
	: d(std::make_unique<Private>(this)) {
	load(jsonData);
}

ConsoleJson::~ConsoleJson() = default;


bool ConsoleJson::isValid() const {
	return d->isValid;
}
QString ConsoleJson::getErrorMessage() const {
	return d->errorMessage;
}
bool ConsoleJson::isEmpty() const {
	return d->jsonDoc.isNull();
}

bool ConsoleJson::load(const QString& jsonString) {
	if (jsonString.trimmed().isEmpty()) {
		d->isValid = false;
		d->errorMessage = "JSON строка пуста";
		return false;
	}

	QJsonParseError error;
	d->jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);

	if (error.error != QJsonParseError::NoError) {
		d->isValid = false;
		d->errorMessage = QString("Ошибка парсинга JSON: %1 (позиция %2)")
			.arg(error.errorString())
			.arg(error.offset);
		return false;
	}

	d->isValid = true;
	d->errorMessage.clear();

	if (d->jsonDoc.isObject()) {
		d->rootNode = d->buildNode(d->jsonDoc.object(), "root");
	}
	else if (d->jsonDoc.isArray()) {
		d->rootNode = d->buildNode(d->jsonDoc.array(), "root");
	}
	else {
		d->rootNode = d->buildNode(d->jsonDoc.object(), "root");
	}

	return true;
}

bool ConsoleJson::load(const QJsonDocument& jsonDoc) {
	if (jsonDoc.isNull()) {
		d->isValid = false;
		d->errorMessage = "JSON документ пуст";
		return false;
	}

	d->jsonDoc = jsonDoc;
	d->isValid = true;
	d->errorMessage.clear();

	if (d->jsonDoc.isObject()) {
		d->rootNode = d->buildNode(d->jsonDoc.object(), "root");
	}
	else if (d->jsonDoc.isArray()) {
		d->rootNode = d->buildNode(d->jsonDoc.array(), "root");
	}
	else {
		d->rootNode = d->buildNode(d->jsonDoc);
	}

	return true;
}

bool ConsoleJson::load(const QByteArray& jsonData) {
	if (jsonData.trimmed().isEmpty()) {
		d->isValid = false;
		d->errorMessage = "JSON данные пусты";
		return false;
	}

	QJsonParseError error;
	d->jsonDoc = QJsonDocument::fromJson(jsonData, &error);

	if (error.error != QJsonParseError::NoError) {
		d->isValid = false;
		d->errorMessage = QString("Ошибка парсинга JSON: %1 (позиция %2)")
			.arg(error.errorString())
			.arg(error.offset);
		return false;
	}

	d->isValid = true;
	d->errorMessage.clear();

	d->rootNode = d->buildNode(d->jsonDoc, "root");

	return true;
}

void ConsoleJson::Private::setStyleConfig(const JsonStyleConfig& config) {
	style = config;
}

JsonNode ConsoleJson::Private::buildNode(const QJsonDocument& doc, const QString& key, int depth) const {
	JsonNode node;
	node.key = key;
	node.depth = depth;

	if (doc.isNull()) {
		node.type = "null";
		node.value = "null";
		return node;
	}

	if (doc.isObject()) {
		node.type = "object";
		QJsonObject obj = doc.object();
		for (auto it = obj.begin(); it != obj.end(); ++it) {
			node.children.append(buildNode(it.value(), it.key(), depth + 1));
		}
		node.value = QString("{%1}").arg(obj.size());
	}
	else if (doc.isArray()) {
		node.type = "array";
		QJsonArray arr = doc.array();
		if (style.collapseLongArrays && arr.size() > style.maxArrayItems) {
			for (int i = 0; i < qMin(arr.size(), style.maxArrayItems); ++i) {
				node.children.append(buildNode(arr[i], QString::number(i), depth + 1));
			}
			// Добавляем информацию о пропущенных элементах
			JsonNode moreNode;
			moreNode.key = "...";
			moreNode.value = QString("и еще %1 элементов").arg(arr.size() - style.maxArrayItems);
			moreNode.type = "more";
			moreNode.depth = depth + 1;
			node.children.append(moreNode);
		}
		else {
			for (int i = 0; i < arr.size(); ++i) {
				node.children.append(buildNode(arr[i], QString::number(i), depth + 1));
			}
		}
		node.value = QString("[%1]").arg(arr.size());
	}
	else {
		// Для простых значений (строка, число, булево, null)
		QJsonValue value = doc.toVariant().toJsonValue();
		node = buildNode(value, key, depth);
	}

	return node;
}

JsonNode ConsoleJson::Private::buildNode(const QJsonValue& value, const QString& key, int depth) const {
	JsonNode node;
	node.key = key;
	node.depth = depth;

	switch (value.type()) {
	case QJsonValue::Object: {
		node.type = "object";
		QJsonObject obj = value.toObject();
		for (auto it = obj.begin(); it != obj.end(); it++) {
			node.children.append(buildNode(it.value(), it.key(), depth + 1));
		}
		node.value = QString("{%1}").arg(obj.size());
		break;
	}
	case QJsonValue::Array: {
		node.type = "array";
		QJsonArray arr = value.toArray();
		if (style.collapseLongArrays && arr.size() > style.maxArrayItems) {
			for (int i = 0; i < qMin(arr.size(), style.maxArrayItems); i++) {
				node.children.append(buildNode(arr[i], QString::number(i), depth + 1));
			}
			// Добавляем информацию о пропущенных элементах
			JsonNode moreNode;
			moreNode.key = "...";
			moreNode.value = QString("и еще %1 элементов").arg(arr.size() - style.maxArrayItems);
			moreNode.type = "more";
			moreNode.depth = depth + 1;
			node.children.append(moreNode);
		}
		else {
			for (int i = 0; i < arr.size(); i++) {
				node.children.append(buildNode(arr[i], QString::number(i), depth + 1));
			}
		}
		node.value = QString("[%1]").arg(arr.size());
		break;
	}
	case QJsonValue::String: {
		node.type = "string";
		node.value = value.toString();
		break;
	}
	case QJsonValue::Double: {
		node.type = "number";
		double num = value.toDouble();
		if (num == int(num)) {
			node.value = QString::number(int(num));
		}
		else {
			node.value = QString::number(num);
		}
		break;
	}
	case QJsonValue::Bool: {
		node.type = "boolean";
		node.value = value.toBool() ? "true" : "false";
		break;
	}
	case QJsonValue::Null: {
		node.type = "null";
		node.value = "null";
		break;
	}
	case QJsonValue::Undefined: {
		node.type = "undefined";
		node.value = "undefined";
		break;
	}
	}

	return node;
}

QString ConsoleJson::Private::escapeJsonString(const QString& str) const {
	QString escaped = str;
	escaped.replace("\\", "\\\\");
	escaped.replace("\"", "\\\"");
	escaped.replace("\n", "\\n");
	escaped.replace("\r", "\\r");
	escaped.replace("\t", "\\t");
	return escaped;
}

QString ConsoleJson::Private::getIndent(int depth) const {
	return QString("&nbsp;&nbsp;").repeated(depth);
}
QString ConsoleJson::Private::renderValue(const QJsonValue& value, int depth, const QString& key) const {
	QString html;
	QString indent = getIndent(depth);
	QString valueType = getValueType(value);

	// Если есть ключ, рендерим его
	if (!key.isEmpty() && key != "root") {
		html += QString("<span class=\"json-key\" style=\"color: %1;\">\"%2\"</span>")
			.arg(style.keyColor)
			.arg(escapeJsonString(key));
		html += QString("<span class=\"json-punctuation\" style=\"color: %1;\">: </span>")
			.arg(style.punctuationColor);
	}

	// Рендерим значение в зависимости от типа
	if (valueType == "object") {
		QJsonObject obj = value.toObject();
		if (obj.isEmpty()) {
			html += QString("<span class=\"json-bracket\" style=\"color: %1;\">{}</span>")
				.arg(style.bracketColor);
		}
		else {
			html += QString("<span class=\"json-bracket\" style=\"color: %1;\">{</span>")
				.arg(style.bracketColor);
			html += "<div class=\"json-children\">";

			// Рендерим все свойства объекта
			bool first = true;
			for (auto it = obj.begin(); it != obj.end(); ++it) {
				if (!first) {
					html += QString("<span class=\"json-punctuation\" style=\"color: %1;\">,</span>")
						.arg(style.punctuationColor);
				}
				html += renderValue(it.value(), depth + 1, it.key());
				first = false;
			}

			html += "</div>";
			html += QString("<span class=\"json-bracket\" style=\"color: %1;\">}</span>")
				.arg(style.bracketColor);
		}
	}
	else if (valueType == "array") {
		QJsonArray arr = value.toArray();
		if (arr.isEmpty()) {
			html += QString("<span class=\"json-bracket\" style=\"color: %1;\">[]</span>")
				.arg(style.bracketColor);
		}
		else {
			// Проверяем, нужно ли сворачивать длинный массив
			bool shouldCollapse = style.collapseLongArrays && arr.size() > style.maxArrayItems;
			int itemsToShow = shouldCollapse ? style.maxArrayItems : arr.size();

			html += QString("<span class=\"json-bracket\" style=\"color: %1;\">[</span>")
				.arg(style.bracketColor);
			html += "<div class=\"json-children\">";

			// Рендерим элементы массива
			for (int i = 0; i < itemsToShow; ++i) {
				if (i > 0) {
					html += QString("<span class=\"json-punctuation\" style=\"color: %1;\">,</span>")
						.arg(style.punctuationColor);
				}
				html += renderValue(arr[i], depth + 1);
			}

			// Если массив свернут, добавляем информацию о пропущенных элементах
			if (shouldCollapse) {
				html += QString("<span class=\"json-punctuation\" style=\"color: %1;\">, </span>")
					.arg(style.punctuationColor);
				html += QString("<span class=\"json-more\" style=\"color: %1; font-style: italic;\">... и еще %2 элементов</span>")
					.arg(style.commentColor)
					.arg(arr.size() - style.maxArrayItems);
			}

			html += "</div>";
			html += QString("<span class=\"json-bracket\" style=\"color: %1;\">]</span>")
				.arg(style.bracketColor);
		}
	}
	else if (valueType == "string") {
		html += QString("<span class=\"json-string\" style=\"color: %1;\">\"%2\"</span>")
			.arg(style.stringColor)
			.arg(escapeJsonString(value.toString()));
	}
	else if (valueType == "number") {
		double num = value.toDouble();
		QString numStr = (num == int(num)) ? QString::number(int(num)) : QString::number(num);
		html += QString("<span class=\"json-number\" style=\"color: %1;\">%2</span>")
			.arg(style.numberColor)
			.arg(numStr);
	}
	else if (valueType == "boolean") {
		html += QString("<span class=\"json-boolean\" style=\"color: %1;\">%2</span>")
			.arg(style.booleanColor)
			.arg(value.toBool() ? "true" : "false");
	}
	else if (valueType == "null") {
		html += QString("<span class=\"json-null\" style=\"color: %1;\">null</span>")
			.arg(style.nullColor);
	}
	else {
		html += QString("<span style=\"color: %1;\">undefined</span>")
			.arg(style.punctuationColor);
	}

	return html;
}

QString ConsoleJson::Private::getValueType(const QJsonValue& value) const {
	switch (value.type()) {
	case QJsonValue::Object:
	return "object";
	case QJsonValue::Array:
	return "array";
	case QJsonValue::String:
	return "string";
	case QJsonValue::Double:
	return "number";
	case QJsonValue::Bool:
	return "boolean";
	case QJsonValue::Null:
	return "null";
	case QJsonValue::Undefined:
	return "undefined";
	}
	return "unknown";
}

QString ConsoleJson::Private::renderNode(const JsonNode& node, int depth, bool isLast) const {
	QString html;
	QString indent = getIndent(depth);

	// Открываем div для узла
	html += QString("<div class=\"json-node\" style=\"padding-left: %1;\">")
		.arg(indent.isEmpty() ? "0px" : style.indentSize);

	// Рендерим ключ (если есть и не root)
	if (!node.key.isEmpty() && node.key != "root") {
		html += QString("<span class=\"json-key\" style=\"color: %1;\">\"%2\"</span>")
			.arg(style.keyColor)
			.arg(escapeJsonString(node.key));
		html += QString("<span class=\"json-punctuation\" style=\"color: %1;\">: </span>")
			.arg(style.punctuationColor);
	}

	// Рендерим значение в зависимости от типа
	if (node.type == "object" || node.type == "array") {
		// Открывающая скобка
		QString bracketOpen = (node.type == "object") ? "{" : "[";
		html += QString("<span class=\"json-bracket\" style=\"color: %1;\">%2</span>")
			.arg(style.bracketColor)
			.arg(bracketOpen);

		// Рендерим детей
		if (!node.children.isEmpty()) {
			html += "<div class=\"json-children\">";
			for (int i = 0; i < node.children.size(); i++) {
				html += renderNode(node.children[i], depth + 1, i == node.children.size() - 1);
			}
			html += "</div>";
		}

		// Закрывающая скобка
		QString bracketClose = (node.type == "object") ? "}" : "]";
		html += QString("<span class=\"json-bracket\" style=\"color: %1;\">%2</span>")
			.arg(style.bracketColor)
			.arg(bracketClose);

	}
	else if (node.type == "more") {
		// Специальный узел для пропущенных элементов
		html += QString("<span class=\"json-more\" style=\"color: %1; font-style: italic;\">%2</span>")
			.arg(style.commentColor)
			.arg(node.value);
	}
	else {
		// Примитивное значение
		QString color;
		if (node.type == "string") {
			color = style.stringColor;
			html += QString("<span class=\"json-string\" style=\"color: %1;\">\"%2\"</span>")
				.arg(color)
				.arg(escapeJsonString(node.value));
		}
		else if (node.type == "number") {
			color = style.numberColor;
			html += QString("<span class=\"json-number\" style=\"color: %1;\">%2</span>")
				.arg(color)
				.arg(node.value);
		}
		else if (node.type == "boolean") {
			color = style.booleanColor;
			html += QString("<span class=\"json-boolean\" style=\"color: %1;\">%2</span>")
				.arg(color)
				.arg(node.value);
		}
		else if (node.type == "null") {
			color = style.nullColor;
			html += QString("<span class=\"json-null\" style=\"color: %1;\">%2</span>")
				.arg(color)
				.arg(node.value);
		}
		else {
			color = style.punctuationColor;
			html += QString("<span style=\"color: %1;\">%2</span>")
				.arg(color)
				.arg(node.value);
		}
	}

	html += "</div>";
	return html;
}

QString ConsoleJson::toHtml() const {
	return toHtml("json-viewer");
}

QString ConsoleJson::toHtml(const QString& styleClass) const {
	if (!d->isValid) {
		return QString("<div class=\"%1-error\" style=\"color: #f44336; padding: 10px; background-color: #1e1e1e; border-radius: 3px;\">"
			"<strong>Ошибка JSON:</strong> %2</div>")
			.arg(styleClass)
			.arg(d->errorMessage.toHtmlEscaped());
	}

	if (d->jsonDoc.isNull()) {
		return QString("<div class=\"%1-empty\" style=\"color: #808080; padding: 10px; background-color: #1e1e1e; border-radius: 3px;\">"
			"<em>Нет данных для отображения</em></div>")
			.arg(styleClass);
	}

	QString html = QString("<div class=\"%1\" style=\"background-color: %2; font-family: %3; font-size: %4; padding: 10px; border-radius: 3px; overflow-x: auto; line-height: 1.6;\">")
		.arg(styleClass)
		.arg(d->style.backgroundColor)
		.arg(d->style.fontFamily)
		.arg(d->style.fontSize);

	// Рендерим корневой узел
	if (d->jsonDoc.isObject() || d->jsonDoc.isArray()) {
		html += d->renderNode(d->rootNode, 0);
	}
	else {
		// Для простых значений
		QString value = d->jsonDoc.toJson(QJsonDocument::Indented);
		html += QString("<pre style=\"margin: 0; color: %1;\">%2</pre>")
			.arg(d->style.stringColor)
			.arg(value.toHtmlEscaped());
	}

	html += "</div>";
	return html;
}
