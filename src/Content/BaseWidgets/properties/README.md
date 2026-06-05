## Примеры использования

# Комбобокс
```cpp
props.append({
		"shader_type", "Тип шейдера", PropertyType::Combo,
		"Vertex", QVariant(), false, {"Vertex", "Fragment", "Combine"},
		"", "Выберите тип компиляции"
	});
```

# Текстовое поле (read-only)
```cpp
props.append({
    "id_guid", "GUID", PropertyType::Text,
    node->guid(), QVariant(), true, QStringList(),
    "", "Идентификатор (нельзя изменить)"
	});
```

# Булево значение
```cpp
props.append({
		"is_enabled", "Включено", PropertyType::Boolean,
		node->isEnabled(), QVariant(), false
	});
```

# Число
```cpp
props.append({
		"opacity", "Прозрачность", PropertyType::Number,
		node->opacity(), QVariant(), false, QStringList(),
		"", 0.0, 1.0, 0.05
	});
```

# Путь к файлу
```cpp
props.append({
		"path", "Путь к файлу", PropertyType::PathFile,
		node.path(), QVariant(), false, QStringList(),
		"Images (*.png *.jpg *.tga)", ""
	});
```

# Кнопка
```cpp
props.append({
		"edit", "Редактировать", PropertyType::Button,
		QVariant(), QVariant(), false, QStringList(),
		QString(),
		"🖼️ Редактировать шейдер"
	});
```

# Выбор цвета
```cpp
props.append({
		"main_color", "Основной цвет", PropertyType::Color,
		QColor("#3A7BD5"), QVariant(), false
	});
```
