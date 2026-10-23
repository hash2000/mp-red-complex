#pragma once
#include <QStringList>
#include <vector>

class Resources;

class Bip39 {
public:
	Bip39(Resources* resources);
	virtual ~Bip39();

	// Генерация мнемонической фразы (128 бит = 12 слов)
	QStringList generateMnemonic(int strengthBits = 128);

	// Преобразование фразы в 64-байтовый мастер-сид (для вывода ключей)
	QByteArray mnemonicToSeed(const QStringList& words, const QString& passphrase = { }) const;

	// Проверка валидности фразы (возвращает энтропию, если фраза корректна)
	std::vector<uint8_t> validateAndGetEntropy(const QStringList& words) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
