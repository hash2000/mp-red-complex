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
stream palette {
  // Базовые цвета
  vec[rgb] button: (35, 35, 35)
  vec[rgb] window: (50, 50, 50)
  vec[rgb] text: (207, 207, 207)

  // Производные цвета (вычисляются рантаймом по атрибуту darker)
  vec[rgb, darker=200] buttonDark: {{.button}}
  vec[rgb, darker=200] windowDark: {{.window}}
  vec[rgb, darker=200] textDark: {{.text}}
}
```

```
// palette-mapper.processor
processor generate-palette {
  use {
    // Принимаем на вход поток с именем palette
    palette input
  }

  export {
    // Отдаём поток с маппингом состояний
    mapping ui-states
  }

  // Логика маппинга
  mapping ui-states {
    Window[Active]: {{.input.window}}
    Window[Disabled]: {{.input.windowDark}}
    Window[Inactive]: (40, 40, 40)  // Хардкод для специфичного состояния

    Base: {{.input.buttonDark}}

    ButtonText[Active]: {{.input.text}}
    ButtonText[Inactive]: {{.input.text}}
    ButtonText[Disabled]: {{.input.textDark}}
  }
}
```
