#include "Content/UsersModule/services/register_user_service.h"
#include "Content/UsersModule/context/current_user_context.h"
//#include "Content/UsersModule/crypto/bip_39_wrapper.h"
#include <QJsonObject>
#include <QCryptographicHash>
#include <QDir>

class RegisterUserService::Private {
public:
	Private(RegisterUserService* parent) : q(parent) {}
	RegisterUserService* q;
	Resources* resources;
};

RegisterUserService::RegisterUserService(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

RegisterUserService::~RegisterUserService() = default;

bool RegisterUserService::registerUser(
	const QString& login,
	const QString& password,
	const QString& displayName) {

	// 1. Генерируем seed-фразу (12 слов)
	//Bip39Wrapper bip39;
	//outSeedPhrase = bip39.generateMnemonic(128);  // 128 бит = 12 слов

	//// 2. Из seed-фразы получаем мастер-сид (64 байта)
	//QByteArray masterSeed = bip39.mnemonicToSeed(outSeedPhrase);

	//// 3. Вычисляем user_hash (SHA256 от мастер-сида)
	//std::vector<unsigned char> hash(32);
	//crypto_hash_sha256(hash.data(),
	//	reinterpret_cast<const unsigned char*>(masterSeed.constData()),
	//	masterSeed.size());
	//QString userHash = QByteArray::fromRawData(
	//	reinterpret_cast<const char*>(hash.data()), 32
	//).toHex();

	//// 4. Генерируем соль для шифрования seed-фразы
	//std::vector<unsigned char> salt(crypto_pwhash_SALTBYTES);
	//randombytes_buf(salt.data(), salt.size());

	//// 5. Выводим ключ для шифрования seed-фразы из пароля
	//std::vector<unsigned char> enc_key(crypto_box_SEEDBYTES);
	//QByteArray pwdUtf8 = password.toUtf8();

	//if (crypto_pwhash(enc_key.data(), enc_key.size(),
	//	pwdUtf8.constData(), pwdUtf8.length(),
	//	salt.data(),
	//	crypto_pwhash_OPSLIMIT_SENSITIVE,
	//	crypto_pwhash_MEMLIMIT_SENSITIVE,
	//	crypto_pwhash_ALG_ARGON2ID13) != 0) {
	//	sodium_memzero(masterSeed.data(), masterSeed.size());
	//	return false;
	//}

	//// 6. Шифруем seed-фразу (XChaCha20-Poly1305)
	//std::vector<unsigned char> nonce(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
	//randombytes_buf(nonce.data(), nonce.size());

	//QByteArray seedPhraseBytes = outSeedPhrase.join(' ').toUtf8();
	//std::vector<unsigned char> ciphertext(
	//	seedPhraseBytes.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES
	//);
	//unsigned long long ciphertext_len;

	//crypto_aead_xchacha20poly1305_ietf_encrypt(
	//	ciphertext.data(), &ciphertext_len,
	//	reinterpret_cast<const unsigned char*>(seedPhraseBytes.constData()),
	//	seedPhraseBytes.size(),
	//	nullptr, 0, nullptr,
	//	nonce.data(),
	//	enc_key.data()
	//);

	//// 7. Выводим ключ для БД (детерминированно из мастер-сида)
	//std::vector<unsigned char> db_key(crypto_box_SEEDBYTES);
	//crypto_kdf_derive_from_key(db_key.data(), db_key.size(),
	//	1,              // индекс подраздела (1 = БД)
	//	"appdbkey",     // контекст (8 байт)
	//	reinterpret_cast<const unsigned char*>(masterSeed.constData()));

	//// 8. Сохраняем зашифрованную seed-фразу
	//QString userDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
	//	+ "/users/" + userHash;
	//QDir().mkpath(userDir);

	//QFile file(userDir + "/auth.enc");
	//if (!file.open(QIODevice::WriteOnly)) {
	//	sodium_memzero(masterSeed.data(), masterSeed.size());
	//	sodium_memzero(enc_key.data(), enc_key.size());
	//	sodium_memzero(db_key.data(), db_key.size());
	//	return false;
	//}

	//// Формат: [salt (16 байт)] [nonce (24 байта)] [ciphertext]
	//file.write(QByteArray::fromRawData(reinterpret_cast<const char*>(salt.data()), salt.size()));
	//file.write(QByteArray::fromRawData(reinterpret_cast<const char*>(nonce.data()), nonce.size()));
	//file.write(QByteArray::fromRawData(reinterpret_cast<const char*>(ciphertext.data()), ciphertext_len));
	//file.close();

	//// 9. Устанавливаем контекст пользователя
	//UserContext::instance().setCurrentUser(
	//	userHash,
	//	QByteArray::fromRawData(reinterpret_cast<const char*>(db_key.data()), db_key.size())
	//);

	//sodium_memzero(masterSeed.data(), masterSeed.size());
	//sodium_memzero(enc_key.data(), enc_key.size());
	//sodium_memzero(db_key.data(), db_key.size());

	return true;
}
