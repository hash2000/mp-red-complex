## Общее

Собственный парсер 

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
