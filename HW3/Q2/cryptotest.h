#ifndef CRYPTOTEST_H
#define CRYPTOTEST_H

#include <QObject>
#include <QVector>
#include <QByteArray>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

class CryptoTest : public QObject
{
    Q_OBJECT
public:
    explicit CryptoTest(QObject *parent = nullptr);
    ~CryptoTest();

    QVector<double> testAES(const QVector<QByteArray>& messages);
    QVector<double> testRSA(const QVector<QByteArray>& messages);
    QVector<QByteArray> generateRandomMessages(int count, int size);

private:
    RSA* rsaKeyPair;
    unsigned char aesKey[16]; // 128-bit AES key
    void initializeKeys();
};

#endif // CRYPTOTEST_H
