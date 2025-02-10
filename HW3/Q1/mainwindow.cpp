#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QTableWidget>

#include <gpgme.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->importKeyButton, &QPushButton::clicked, this, &MainWindow::on_importKeyButton_clicked);
    connect(ui->encryptTextButton, &QPushButton::clicked, this, &MainWindow::on_encryptTextButton_clicked);
    connect(ui->encryptFileButton, &QPushButton::clicked, this, &MainWindow::on_encryptFileButton_clicked);
    connect(ui->decryptButton, &QPushButton::clicked, this, &MainWindow::on_decryptButton_clicked);

    initializeLogTable();

    gpgme_check_version(NULL);

    updatePublicKeyList();
    updatePrivateKeyList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeLogTable() {
    ui->logTable->setColumnCount(2);
    QStringList headers;
    headers << "Message" << "Timestamp";
    ui->logTable->setHorizontalHeaderLabels(headers);
}

void MainWindow::addLogEntry(const QString &message) {
    int row = ui->logTable->rowCount();
    ui->logTable->insertRow(row);

    QTableWidgetItem *timestampItem = new QTableWidgetItem(QDateTime::currentDateTime().toString());
    QTableWidgetItem *messageItem = new QTableWidgetItem(message);

    ui->logTable->setItem(row, 0, messageItem);
    ui->logTable->setItem(row, 1, timestampItem);
}

void MainWindow::updatePublicKeyList()
{
    ui->publicKeyList->clear();

    gpgme_ctx_t ctx;
    gpgme_key_t key;
    gpgme_error_t err;

    err = gpgme_new(&ctx);
    if (err) {
        addLogEntry(QString("Failed to initialize GPGME: %1").arg(gpgme_strerror(err)));
        return;
    }

    err = gpgme_op_keylist_start(ctx, NULL, 0);
    if (err) {
        addLogEntry(QString("Failed to start key listing: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    while (!(err = gpgme_op_keylist_next(ctx, &key))) {
        if (key->can_encrypt) {
            QListWidgetItem *item = new QListWidgetItem;
            item->setText(QString::fromUtf8(key->uids->email));
            ui->publicKeyList->addItem(item);
        }
        gpgme_key_release(key);
    }

    if (err && gpgme_err_code(err) != GPG_ERR_EOF) {
        addLogEntry(QString("Failed to list keys: %1").arg(gpgme_strerror(err)));
    }

    gpgme_op_keylist_end(ctx);
    gpgme_release(ctx);
}

void MainWindow::updatePrivateKeyList()
{
    ui->privateKeyList->clear();

    gpgme_ctx_t ctx;
    gpgme_key_t key;
    gpgme_error_t err;

    err = gpgme_new(&ctx);
    if (err) {
        addLogEntry(QString("Failed to initialize GPGME: %1").arg(gpgme_strerror(err)));
        return;
    }

    err = gpgme_op_keylist_start(ctx, NULL, 1);
    if (err) {
        addLogEntry(QString("Failed to start key listing: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    while (!(err = gpgme_op_keylist_next(ctx, &key))) {
        if (key->can_sign) {
            QListWidgetItem *item = new QListWidgetItem;
            item->setText(QString::fromUtf8(key->uids->email));
            ui->privateKeyList->addItem(item);
        }
        gpgme_key_release(key);
    }

    if (err && gpgme_err_code(err) != GPG_ERR_EOF) {
        addLogEntry(QString("Failed to list keys: %1").arg(gpgme_strerror(err)));
    }

    gpgme_op_keylist_end(ctx);
    gpgme_release(ctx);
}

void MainWindow::on_importKeyButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr, "Select Key File", "", "Key Files (*.asc *.gpg *.pgp);;All Files (*)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        addLogEntry(QString("Unable to open file %1").arg(fileName));
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    gpgme_ctx_t ctx = nullptr;
    gpgme_data_t keyData = nullptr;
    gpgme_error_t err;

    err = gpgme_new(&ctx);
    if (err) {
        addLogEntry(QString("Failed to initialize GPGME context: %1").arg(gpgme_strerror(err)));
        return;
    }

    err = gpgme_data_new_from_mem(&keyData, fileData.constData(), fileData.size(), 0);
    if (err) {
        addLogEntry(QString("Failed to create GPGME data object: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    err = gpgme_op_import(ctx, keyData);
    if (err) {
        addLogEntry(QString("Failed to import keys: %1").arg(gpgme_strerror(err)));
    } else {
        gpgme_import_result_t result = gpgme_op_import_result(ctx);
        QString message = QString("Imported: %1, Unchanged: %2, Not imported: %3")
                              .arg(result->imported)
                              .arg(result->unchanged)
                              .arg(result->not_imported);
        addLogEntry(message);
    }

    gpgme_data_release(keyData);
    gpgme_release(ctx);

    updatePublicKeyList();
    updatePrivateKeyList();
}

void MainWindow::on_encryptTextButton_clicked()
{
    gpgme_ctx_t ctx = nullptr;
    gpgme_key_t pubkey = nullptr, privkey = nullptr;
    gpgme_error_t err;
    gpgme_data_t plain = nullptr, encrypted = nullptr;

    QString plainText = ui->textLE->text();
    ui->textLE->clear();
    if (plainText.isEmpty()) {
        return;
    }

    if (!ui->privateKeyList->currentItem() || !ui->publicKeyList->currentItem())
    {
        addLogEntry("Please select both public and private keys");
        return;
    }
    QString privateEmail = ui->privateKeyList->currentItem()->text();
    QString publicEmail = ui->publicKeyList->currentItem()->text();

    err = gpgme_new(&ctx);
    if (err) {
        addLogEntry(QString("Failed to initialize GPGME context: %1").arg(gpgme_strerror(err)));
        return;
    }

    err = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
    if (err) {
        addLogEntry(QString("Failed to set GPGME protocol: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    err = gpgme_get_key(ctx, publicEmail.toUtf8().constData(), &pubkey, 0);
    if (err) {
        addLogEntry(QString("Unable to get Public Key: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    err = gpgme_get_key(ctx, privateEmail.toUtf8().constData(), &privkey, 1);
    if (err) {
        addLogEntry(QString("Unable to get Private Key: %1").arg(gpgme_strerror(err)));
        gpgme_key_unref(pubkey);
        gpgme_release(ctx);
        return;
    }

    err = gpgme_data_new_from_mem(&plain, plainText.toUtf8().constData(), plainText.size(), 1);
    if (err) {
        addLogEntry(QString("Failed to create plain text data object: %1").arg(gpgme_strerror(err)));
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    err = gpgme_data_new(&encrypted);
    if (err) {
        addLogEntry(QString("Failed to create encrypted data object: %1").arg(gpgme_strerror(err)));
        gpgme_data_release(plain);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    err = gpgme_signers_add(ctx, privkey);
    if (err) {
        addLogEntry(QString("Failed to add signer: %1").arg(gpgme_strerror(err)));
        gpgme_data_release(plain);
        gpgme_data_release(encrypted);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    gpgme_key_t keys[] = {pubkey, nullptr};
    err = gpgme_op_encrypt_sign(ctx, keys, GPGME_ENCRYPT_ALWAYS_TRUST, plain, encrypted);
    if (err) {
        addLogEntry(QString("Encryption and signing failed").arg(gpgme_strerror(err)));
        gpgme_data_release(plain);
        gpgme_data_release(encrypted);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    QString filename = QString("%1.gpg").arg(timestamp);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        addLogEntry("Failed to open the file for writing");
        gpgme_data_release(plain);
        gpgme_data_release(encrypted);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    QByteArray encryptedData;
    char buffer[512];
    int n;

    if (gpgme_data_seek(encrypted, 0, SEEK_SET) == -1)
    {
        addLogEntry("Failed to seek gpgme data object");
        gpgme_data_release(plain);
        gpgme_data_release(encrypted);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    while ((n = gpgme_data_read(encrypted, buffer, sizeof(buffer))) > 0) {
        encryptedData.append(buffer, n);
    }

    file.write(encryptedData);
    file.close();

    addLogEntry("The file has been successfully encrypted and saved as " + filename);

    gpgme_data_release(plain);
    gpgme_data_release(encrypted);
    gpgme_key_unref(pubkey);
    gpgme_key_unref(privkey);
    gpgme_release(ctx);
}

void MainWindow::on_encryptFileButton_clicked()
{
    gpgme_ctx_t ctx = nullptr;
    gpgme_key_t pubkey = nullptr, privkey = nullptr;
    gpgme_error_t err;
    gpgme_data_t plain = nullptr, encrypted = nullptr;

    QString fileName = QFileDialog::getOpenFileName(nullptr, "Select File");
    if (fileName.isEmpty()) {
        return;
    }

    QFile plainFile(fileName);
    if (!plainFile.open(QIODevice::ReadOnly)) {
        addLogEntry("Unable to Open File " + fileName);
        return;
    }

    QByteArray plainData = plainFile.readAll();
    plainFile.close();

    if (!ui->privateKeyList->currentItem() || !ui->publicKeyList->currentItem())
    {
        addLogEntry("Please select both public and private keys");
        return;
    }
    QString privateEmail = ui->privateKeyList->currentItem()->text();
    QString publicEmail = ui->publicKeyList->currentItem()->text();

    err = gpgme_new(&ctx);
    if (err) {
        addLogEntry(QString("Failed to initialize GPGME context: %1").arg(gpgme_strerror(err)));
        return;
    }

    err = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
    if (err) {
        addLogEntry(QString("Failed to set GPGME protocol: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    err = gpgme_get_key(ctx, publicEmail.toUtf8().constData(), &pubkey, 0);
    if (err) {
        addLogEntry(QString("Unable to get Public Key: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    err = gpgme_get_key(ctx, privateEmail.toUtf8().constData(), &privkey, 1);
    if (err) {
        addLogEntry(QString("Unable to get Private Key: %1").arg(gpgme_strerror(err)));
        gpgme_key_unref(pubkey);
        gpgme_release(ctx);
        return;
    }

    err = gpgme_data_new_from_mem(&plain, plainData.constData(), plainData.size(), 1);
    if (err) {
        addLogEntry(QString("Failed to create plain text data object: %1").arg(gpgme_strerror(err)));
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    err = gpgme_data_new(&encrypted);
    if (err) {
        addLogEntry(QString("Failed to create encrypted data object: %1").arg(gpgme_strerror(err)));
        gpgme_data_release(plain);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    err = gpgme_signers_add(ctx, privkey);
    if (err) {
        addLogEntry(QString("Failed to add signer: %1").arg(gpgme_strerror(err)));
        gpgme_data_release(plain);
        gpgme_data_release(encrypted);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    gpgme_key_t keys[] = {pubkey, nullptr};
    err = gpgme_op_encrypt_sign(ctx, keys, GPGME_ENCRYPT_ALWAYS_TRUST, plain, encrypted);
    if (err) {
        addLogEntry(QString("Encryption and signing failed: %1").arg(gpgme_strerror(err)));
        gpgme_data_release(plain);
        gpgme_data_release(encrypted);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    QString filename = QString("%1.gpg").arg(timestamp);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        addLogEntry("Failed to open the file for writing");
        gpgme_data_release(plain);
        gpgme_data_release(encrypted);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    QByteArray encryptedData;
    char buffer[512];
    int n;

    if (gpgme_data_seek(encrypted, 0, SEEK_SET) == -1)
    {
        addLogEntry("Failed to seek gpgme data object");
        gpgme_data_release(plain);
        gpgme_data_release(encrypted);
        gpgme_key_unref(pubkey);
        gpgme_key_unref(privkey);
        gpgme_release(ctx);
        return;
    }

    while ((n = gpgme_data_read(encrypted, buffer, sizeof(buffer))) > 0) {
        encryptedData.append(buffer, n);
    }

    file.write(encryptedData);
    file.close();

    addLogEntry("The file has been successfully encrypted and saved as " + filename);

    gpgme_data_release(plain);
    gpgme_data_release(encrypted);
    gpgme_key_unref(pubkey);
    gpgme_key_unref(privkey);
    gpgme_release(ctx);
}

void MainWindow::on_decryptButton_clicked()
{
    gpgme_ctx_t ctx;
    gpgme_error_t err;
    gpgme_data_t encrypted, plain;

    QString fileName = QFileDialog::getOpenFileName(nullptr, "Select Encrypted File");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        addLogEntry(QString("Unable to Open File: %1").arg(fileName));
        return;
    }

    QByteArray encryptedData = file.readAll();
    file.close();

    err = gpgme_new(&ctx);
    if (err) {
        addLogEntry(QString("Failed to initialize GPGME context: %1").arg(gpgme_strerror(err)));
        return;
    }

    err = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
    if (err) {
        addLogEntry(QString("Failed to set protocol as OpenPGP: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    err = gpgme_data_new_from_mem(&encrypted, encryptedData.constData(), encryptedData.size(), 1);
    if (err) {
        addLogEntry(QString("Failed to create GPGME data object: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    err = gpgme_data_new(&plain);
    if (err) {
        addLogEntry(QString("Failed to create GPGME data object: %1").arg(gpgme_strerror(err)));
        gpgme_data_release(encrypted);
        gpgme_release(ctx);
        return;
    }

    gpgme_decrypt_result_t decrypt_result;
    gpgme_verify_result_t verify_result;
    err = gpgme_op_decrypt_verify(ctx, encrypted, plain);
    if (err) {
        addLogEntry(QString("Failed to decrypt the file: %1").arg(gpgme_strerror(err)));
        gpgme_data_release(encrypted);
        gpgme_data_release(plain);
        gpgme_release(ctx);
        return;
    }

    decrypt_result = gpgme_op_decrypt_result(ctx);
    verify_result = gpgme_op_verify_result(ctx);

    if (decrypt_result->recipients->status != GPG_ERR_NO_ERROR) {
        addLogEntry(QString("Failed to decrypt the file: %1").arg(gpgme_strerror(decrypt_result->recipients->status)));
        gpgme_data_release(encrypted);
        gpgme_data_release(plain);
        gpgme_release(ctx);
        return;
    }

    QString decryptedFileName = fileName + ".decrypted";
    QFile decryptedFile(decryptedFileName);
    if (!decryptedFile.open(QIODevice::WriteOnly)) {
        addLogEntry("Failed to open the file for writing.");
        gpgme_data_release(encrypted);
        gpgme_data_release(plain);
        gpgme_release(ctx);
        return;
    }

    QByteArray decryptedData;
    char buffer[512];
    int n;

    if (gpgme_data_seek(plain, 0, SEEK_SET) == -1)
    {
        addLogEntry("Failed to seek gpgme data object");
        gpgme_data_release(encrypted);
        gpgme_data_release(plain);
        gpgme_release(ctx);
        return;
    }

    while ((n = gpgme_data_read(plain, buffer, sizeof(buffer))) > 0) {
        decryptedData.append(buffer, n);
    }

    decryptedFile.write(decryptedData);
    decryptedFile.close();

    addLogEntry("The file has been successfully decrypted and saved as " + decryptedFileName);

    gpgme_data_release(encrypted);
    gpgme_data_release(plain);
    gpgme_release(ctx);
}
