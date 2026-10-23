#pragma once

#include <QString>
#include <QByteArray>

namespace Extensions::Users::Crypto {
QString computeUserHash(const QByteArray& masterSeed);
QByteArray deriveEncryptionKey(const QString& password, const QByteArray& salt);
QByteArray encryptSeedPhrase(const QString& seedPhrase, const QByteArray& encKey);
QByteArray deriveDatabaseKey(const QByteArray& masterSeed);
QString decryptSeedPhrase(const QByteArray& encryptedData, const QByteArray& encKey);
}
