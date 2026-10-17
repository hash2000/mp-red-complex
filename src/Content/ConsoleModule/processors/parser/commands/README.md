## Пример кода

```
// однострочный комментарий
/*
	Многострочный комментарий
*/


// подключить файл
#include "base/utils.script" 

// типы можно объявлять внутри пространств имён
namespace .base.auth {
	object session { 

		// в объекте session объявить секретную переменную key (её значение не выводится никуда (лог, экран),
		// но можно пользоваться нативно при использовании внутри ICommand). точка с запятой в конце означает ,
		// что переменная объявлена, но значения нет. если стоит двоеточик ':',
		// то дальче читается значение по правилам ниже? точка с запятой в конце не нужна.
		string[mutable, secret, mime("text/plane")] userHash;

		// <<text>><</text>> - многострочный текст
		// "" - однострочный текст, с экранированными символами как в C++
		// <<json>><</json>> - формат json
		// <<xml>><</xml>> - формат xml
		// <<ini>><</ini>> - формат ini

		string[mutable] user; // строковая переменная user

		var description:
		// значение переменной читается от открывающегося тега и до закрывающегося у переменной mime тип "text/plane"	
		<<text>> 
			сессия пользователя
		<</text>>

		var json: 
		// значение переменной читается от открывающегося тега и до закрывающегося у переменной mime тип "application/json"
		// значение переменной хранится как 
		// QJsonDocument doc = QJsonDocument::fromJson(
		// QVariant::fromValue(doc)
		<<json>> 
			{
				"mime": "session",
				"controller": ".session.auth.SessionController"
			}
		<</json>>

		var userPage: 
		// значение переменной читается от открывающегося тега и до закрывающегося у переменной mime тип "application/xml"
		// значение переменной хранится как 
		// QDomDocument doc;
		// QVariant::fromValue(doc)
		<<xml>>
			<head></head>
			<body></body>
		<</xml>>

		var params:
		<<ini>>
		option1:value1
		option2:value2
		<</ini>>
	}

	object tokens {
		string[secret] auth;
		string[secret] refresh;
		string uri;
	}
}

// объявить контекст
// тут не пространство имён для контекста, а вложенность контекстов 
context .system.surrent.user {

	// то что контекст принимает из других контекстов
	import {
		.base.auth.tokens authData;
	}

	// то что контекст вернёт
	export {
		
	}

	// поток контекста
	stream {
		var authAction:"create"; 
		var authTarget:"login";
		var authTitle:"Войти в систему";

		
	}

	// выполнить компнду объявленную class WindowsCommand : public ICommand
	// параметры читаюся "имя параметра":"значение" заканчивается чтение параметров команды ;

	// из потока поместить в команду 
	select
		
	from windows
	where
	// для команды windows, нужно чтобы в контексте были значения параметров
		action: {{authAction}}
		target: {{authTarget}}
		desctiption: {{authTitle}}
;



```
