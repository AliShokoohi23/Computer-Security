#include "cryptotest.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>

// For version checks, you can use these macros if desired
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#include <openssl/conf.h>
#endif

CryptoTest::CryptoTest(QObject *parent) : QObject(parent)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    // For OpenSSL < 1.1.0
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
#else
    // For OpenSSL >= 1.1.0 or 3.0, no extra init needed
#endif

    initializeKeys();
}

CryptoTest::~CryptoTest()
{
    RSA_free(rsaKeyPair);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_cleanup(); // also deprecated in newer OpenSSL
    ERR_free_strings();
#endif
}

void CryptoTest::initializeKeys()
{
    // Generate RSA 3072-bit key
    rsaKeyPair = RSA_new();
    BIGNUM* bne = BN_new();
    BN_set_word(bne, RSA_F4);
    if (!RSA_generate_key_ex(rsaKeyPair, 3072, bne, nullptr)) {
        qWarning() << "RSA key generation failed";
    }
    BN_free(bne);

    // Generate random AES-128 key
    if (!RAND_bytes(aesKey, sizeof(aesKey))) {
        qWarning() << "AES key generation failed";
    }
}

QVector<QByteArray> CryptoTest::generateRandomMessages(int count, int size)
{
    QVector<QByteArray> messages;
    messages.reserve(count);

    for (int i = 0; i < count; i++) {
        QByteArray message(size, 0);
        RAND_bytes(reinterpret_cast<unsigned char*>(message.data()), size);
        messages.append(message);
    }
    return messages;
}

//-----------------------------------------------------------------------------------
//  FRESH AES CONTEXT PER MESSAGE
//-----------------------------------------------------------------------------------
QVector<double> CryptoTest::testAES(const QVector<QByteArray> &messages)
{
    QVector<double> times;
    times.reserve(messages.size());

    for (const QByteArray &msg : messages) {
        // We'll create a fresh context per message
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            qWarning() << "Failed to create cipher context";
            continue;
        }

        // OPTIONAL: use a random IV each time to mimic real usage
        unsigned char iv[16];
        RAND_bytes(iv, sizeof(iv));

        // Start timing
        QElapsedTimer timer;
        timer.start();

        // Initialize for AES-128-CBC
        if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, aesKey, iv) != 1) {
            qWarning() << "AES init failed";
            EVP_CIPHER_CTX_free(ctx);
            continue;
        }

        // We might need some extra space in the output
        QByteArray ciphertext(msg.size() + EVP_MAX_BLOCK_LENGTH, 0);

        int len = 0;
        int ciphertext_len = 0;

        // EncryptUpdate
        if (EVP_EncryptUpdate(ctx,
                              reinterpret_cast<unsigned char*>(ciphertext.data()),
                              &len,
                              reinterpret_cast<const unsigned char*>(msg.constData()),
                              msg.size()) != 1) {
            qWarning() << "AES encrypt update failed";
            EVP_CIPHER_CTX_free(ctx);
            continue;
        }
        ciphertext_len = len;

        // EncryptFinal
        if (EVP_EncryptFinal_ex(ctx,
                                reinterpret_cast<unsigned char*>(ciphertext.data()) + len,
                                &len) != 1) {
            qWarning() << "AES encrypt final failed";
            EVP_CIPHER_CTX_free(ctx);
            continue;
        }
        ciphertext_len += len;
        ciphertext.resize(ciphertext_len);

        // Stop timing
        double elapsedMs = timer.nsecsElapsed() / 1e6; // Convert ns -> ms
        times.push_back(elapsedMs);

        // Free the context
        EVP_CIPHER_CTX_free(ctx);
    }

    return times;
}

QVector<double> CryptoTest::testRSA(const QVector<QByteArray>& messages)
{
    QVector<double> times;
    times.reserve(messages.size());

    int rsa_size = RSA_size(rsaKeyPair);

    for (const QByteArray &msg : messages) {
        QByteArray encrypted(rsa_size, 0);

        // Start timing
        QElapsedTimer timer;
        timer.start();

        int encrypted_length = RSA_public_encrypt(
            qMin(msg.size(), rsa_size - 42), // subtract approx. OAEP overhead
            reinterpret_cast<const unsigned char*>(msg.constData()),
            reinterpret_cast<unsigned char*>(encrypted.data()),
            rsaKeyPair,
            RSA_PKCS1_OAEP_PADDING
            );

        double elapsedMs = timer.nsecsElapsed() / 1e6; // ns->ms

        if (encrypted_length == -1) {
            // Error
            qWarning() << "RSA encryption failed:"
                       << ERR_error_string(ERR_get_error(), nullptr);
            continue;
        }

        times.push_back(elapsedMs);
    }

    return times;
}
