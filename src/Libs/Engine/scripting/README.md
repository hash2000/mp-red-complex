## Пример кода
### Комментарии
```
// однострочный комментарий
/*
	Многострочный комментарий
*/
```
### Типы данных
* `var` - тип определяется по правому выражению
* `string` - строковый тип, `mime type` text/plane
	* `<<text>><</text>>` - строковый тип, многострочный, `mime type` text/plane
	* `<<json>><</json>>` - строковый тип, многострочный, `mime type` application/json
	* `<<xml>><</xml>>` - строковый тип, многострочный, `mime type` text/xml
	* `<<ini>><</ini>>` - строковый тип, многострочный, `mime type` text/x-ini
* `bool` - истина `true`/ложь `false`
* `vec` - векстор (x, y, z)
* `numbeer` - числовое значение
	* поддерживается целый формат
	* поддерживается дробный фломат, разделитель - точка
	* поддерживается разделитель разрядов - нижнее подчёркивание
	* отрицательные значения - перед числом знак минус 
	* положительные значения - перед числом нет знача или знак плюс


### Атрибуты
Пишутся после типа данных к квадратных скобках.
Могут иметь параметры в круглых скобках, перечисленные через запятую.
```
string[attr1, attr2(100, <<text>>
	this is a variable
	has a data type, attributes and name
<</text>>)]
	variable_name;
```

### Переменные
```
// Переменная без значения
string variable

var[attr1, attr2(100, <<text>>
	this is a variable with attributes
	has value of application/json format
<</text>>)]
	variable_name: <<json>>
	{
		"items": [
			"one",
			"two",
			"three"
		] 
	}
	<</json>>;
```

### Биндинг
Чтобы сослаться на значение другой переменной, нужно использовать биндинг - {{сейчас можно привязаять только переменную, дальше, возможно можно будет привязывать на выразение}}.
```
var variable_source: 1000;
var variable_bind: {{.variable_source}} // binding
// roadmap
var variable_bind: {{.variable_source1 | .variable_source2}}

```
Изменение `variable_source` сигнализирует переменной `variable_bind` об обновлении.
Изменение `variable_bind` никак не влияет на `variable_source`

### Подключение скриптов #include
Для подключение других скриптов можно использовать директиву #include.
```
#include "base/utils.script"
```

### Скрипт политры
|Group|
|_____|
|Active|
|Disabled|
|Inactive|
|NColorGroups|
|Current|
|All|
|Normal|


|Role|
|____|
|WindowText|
|Button|
|Light|
|Midlight|
|Dark|
|Mid|
|Text|
|BrightText|
|ButtonText|
|Base|
|Window|
|Shadow|
|Highlight|
|HighlightedText|
|Link|
|LinkVisited|
|AlternateBase|
|NoRole|
|ToolTipBase|
|ToolTipText|
|PlaceholderText|
|Accent|


```
// theme-dark.script
processor application.palette {
	mapping states.ui[
		palette,
		name="dark"
		version="0.0.1",
	] {
		// Build an initial palette based 
		Extends				[ rgb														]:		(35, 35, 35)

		// List box backgrounds, text entry backgrounds, menu backgrounds
		Base					[ rgb,	darker=130							]:		(35, 35, 35)

		// Window colors
		Window				[ rgb,								Active		]:		(50, 50, 50)
		Window				[ rgb,								Inactive	]:		(40, 40, 40)
		Window				[ rgb,	darker=200,		Disabled	]:		(50, 50, 50)

		// Button text
		ButtonText		[ rgb,								Active		]:		(207, 207, 207)
		ButtonText		[ rgb,								Inactive	]:		(207, 207, 207)
		ButtonText		[ rgb,	darker=200,		Disabled	]:		(207, 207, 207)

		// WindowText is supposed to be against QPalette::Window
		WindowText		[ rgb,								Active		]:		(207, 207, 207)
		WindowText		[ rgb,								Inactive	]:		(207, 207, 207)
		WindowText		[ rgb,	darker=200,		Disabled	]:		(207, 207, 207)

		// Menu text, text edit text, table cell text
		Text					[ rgb,	darker=115,		Active		]:		(207, 207, 207)
		Text					[ rgb,	darker=115,		Inactive	]:		(207, 207, 207)
		Text					[ rgb,								Disabled	]:		(102, 102, 102)

		// Disabled menu item text shadow
		Light					[ rgb,	darker=200,		Disabled	]:		(35, 35, 35)

		// Highlight (selected list box row, selected grid cell background, selected tab text
		Highlight			[ rgb,								Active		]:		(62, 112, 205)
		Highlight			[ rgb,								Inactive	]:		(62, 112, 205)
		Highlight			[ rgb,								Disabled	]:		(62, 112, 205)
	}
}
```
