---
name: widget-styling
description: Apply consistent Tailwind Slate color palette and layout patterns to Qt widgets
---

# Widget Styling

Проект использует единую цветовую палитру (Tailwind Slate) для всех виджетов.

## Color Palette

| Элемент | Цвет |
|---------|------|
| Основной фон | `#1a202c` |
| Фон панелей/рамок | `#2d3748` |
| Рамки | `#4a5568` |
| Рамки (hover) | `#718096` |
| Текст основной | `#e2e8f0` |
| Текст вторичный | `#a0aec0` |
| Текст подсказок | `#718096` |

## EquipmentWidget Pattern

```cpp
setObjectName("WidgetName");
setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
setLineWidth(1);

setStyleSheet(R"(
    #WidgetName {
        background-color: #1a202c;
        border: 1px solid #4a5568;
        border-radius: 8px;
        padding: 12px;
    }
)");
```

## Standard Widget Styles

**Заголовок/панель:**
```cpp
"background-color: #2d3748; "
"border: 1px solid #4a5568; "
"border-radius: 4px; "
"padding: 6px;"
```

**Иконки/аватары:**
```cpp
"background-color: #1a202c; "
"border: 1px solid #4a5568; "
"border-radius: 4px;"
```

**Метки с информацией (ID, уровень):**
```cpp
"background-color: #1a202c; "
"color: #a0aec0; "
"padding: 1px 6px; "
"border: 1px solid #4a5568; "
"border-radius: 3px; "
"font-size: 10px; "
"font-family: monospace;"
```

**Кнопки-иконки (UTF-8 символы):**
```cpp
"QPushButton { "
"  background-color: #2d3748; "
"  color: #e2e8f0; "
"  border: 1px solid #4a5568; "
"  border-radius: 3px; "
"  font-size: 12px; "
"}"
"QPushButton:hover { "
"  background-color: #4a5568; "
"  border: 1px solid #718096; "
"}"
"QPushButton:pressed { "
"  background-color: #1a202c; "
"}"
```

**Скроллбар:**
```cpp
"QScrollArea { "
"  background-color: #1a202c; "
"  border: none; "
"  border-left: 2px solid #4a5568; "
"  padding-left: 4px; "
"}"
"QScrollBar:vertical { "
"  background-color: #2d3748; "
"  width: 8px; "
"  border-radius: 4px; "
"}"
"QScrollBar::handle:vertical { "
"  background-color: #4a5568; "
"  border-radius: 4px; "
"  min-height: 20px; "
"}"
"QScrollBar::handle:vertical:hover { "
"  background-color: #718096; "
"}"
"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
"  height: 0px; "
"}"
```

**Разделители:**
```cpp
line->setFrameShape(QFrame::HLine);
line->setStyleSheet("background-color: #2d3748;");
```

## Layout Guidelines

- **Отступы виджетов**: `8px` по краям, `8px` между элементами
- **Отступы записей списка**: `6px` горизонтально, `4px` вертикально
- **Список персонажей**: без рамки, линия слева `border-left: 2px solid #4a5568`
- **Кнопки-иконки**: `24x24`, UTF-8 символы, подсказки через `setToolTip()`
- **Выравнивание**: `Qt::AlignTop` для вертикального расположения элементов

## Example: User Profile Widget

```
UserWidget (QFrame)
├── Header (QFrame, #2d3748)
│   ├── Icon (40x40, #1a202c)
│   └── Info
│       ├── UserId (monospace, #a0aec0)
│       └── DisplayName (bold, #e2e8f0)
└── ScrollArea (#1a202c, border-left only)
    └── CharactersContainer
        ├── CharacterEntryWidget
        │   ├── Icon (48x48)
        │   ├── Info (level + name)
        │   └── Buttons (⚔, 🎒, 24x24, vertical)
        ├── Separator (#2d3748)
        └── CharacterEntryWidget
```
