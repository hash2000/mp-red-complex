#include "Libs/Engine/themes/palette_parser.h"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QDebug>

namespace PaletteParser {
QPalette::ColorRole mapRole(const std::string& roleStr) {
	if (roleStr == "Window") return QPalette::Window;
	if (roleStr == "WindowText") return QPalette::WindowText;
	if (roleStr == "Base") return QPalette::Base;
	if (roleStr == "AlternateBase") return QPalette::AlternateBase;
	if (roleStr == "ToolTipBase") return QPalette::ToolTipBase;
	if (roleStr == "ToolTipText") return QPalette::ToolTipText;
	if (roleStr == "Text") return QPalette::Text;
	if (roleStr == "Button") return QPalette::Button;
	if (roleStr == "ButtonText") return QPalette::ButtonText;
	if (roleStr == "BrightText") return QPalette::BrightText;
	if (roleStr == "Light") return QPalette::Light;
	if (roleStr == "Midlight") return QPalette::Midlight;
	if (roleStr == "Dark") return QPalette::Dark;
	if (roleStr == "Mid") return QPalette::Mid;
	if (roleStr == "Shadow") return QPalette::Shadow;
	if (roleStr == "Highlight") return QPalette::Highlight;
	if (roleStr == "HighlightedText") return QPalette::HighlightedText;

	throw std::runtime_error("Unknown palette role: " + roleStr);
}

QPalette::ColorGroup mapGroup(const std::string& groupStr) {
	if (groupStr == "Active") return QPalette::Active;
	if (groupStr == "Disabled") return QPalette::Disabled;
	if (groupStr == "Inactive") return QPalette::Inactive;
	if (groupStr == "NColorGroups") return QPalette::NColorGroups;
	if (groupStr == "Current") return QPalette::Current;
	if (groupStr == "All") return QPalette::All;

	throw std::runtime_error("Unknown palette group: " + groupStr);
}

QColor parseColor(const YAML::Node& node) {
	if (!node.IsSequence() || node.size() != 3) {
		throw std::runtime_error("Color must be a sequence of 3 integers [R, G, B]");
	}
	int r = node[0].as<int>();
	int g = node[1].as<int>();
	int b = node[2].as<int>();
	return QColor(r, g, b);
}

void applyModifier(QColor& color, const YAML::Node& mods) {
	if (!mods) {
		return;
	}

	if (mods["darker"]) {
		const auto val = mods["darker"].as<int>();
		color.darker(val);
	}

	if (mods["lighter"]) {
		const auto val = mods["lighter"].as<int>();
		color.lighter(val);
	}
}

QPalette parseFromFile(const QString& filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw std::runtime_error("Cannot open palette file: " + filePath.toStdString());
	}

	YAML::Node root = YAML::Load(file.readAll().toStdString());

	if (!root["palette"]) {
		if (!root["palette"]["Extends"]) {
			throw std::runtime_error("Invalid palette structure: missing 'palette.Extends'");
		}

		if (!root["palette"]["colors"]) {
			throw std::runtime_error("Invalid palette structure: missing 'palette.colors'");
		}
	}

	auto palette = QPalette(parseColor(root["palette"]["Extends"]));
	const auto& colors = root["palette"]["colors"];

	if (root["palette"]["Base"]) {
		palette.setColor(QPalette::Base, parseColor(root["palette"]["Base"]));
	}

	for (const auto& rolePair : colors) {
		std::string roleName = rolePair.first.as<std::string>();

		try {
			const auto role = mapRole(roleName);
			const auto& statesNode = rolePair.second;

			// Ожидаем структуру: { value: [r,g,b], modifiers: ... }
			// Пока поддерживаем только прямой value
			for (const auto& statePair : statesNode) {
				const auto stateName = statePair.first.as<std::string>();
				const auto group = mapGroup(stateName);
				const auto& stateData = statePair.second;

				if (!stateData["value"]) {
					qWarning() << "      Missing 'value' in state!" << stateName;
					continue;
				}

				auto color = parseColor(stateData["value"]);
				applyModifier(color, stateData["modifiers"]);

				palette.setColor(group, role, color);
			}
		}
		catch (const std::exception& e) {
			qWarning() << "Skipping unknown or invalid palette role:" << roleName.c_str();
		}
	}

	return palette;
}
}
