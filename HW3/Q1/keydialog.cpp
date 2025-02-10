#include "keydialog.h"
#include "ui_keydialog.h"

#include <QMessageBox>
#include <gpgme.h>

KeyDialog::KeyDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::KeyDialog)
{
    ui->setupUi(this);

    connect(ui->generateKeyButton, &QPushButton::clicked, this, &KeyDialog::on_generateKeyButton_clicked);
    connect(ui->continueButton, &QPushButton::clicked, this, &KeyDialog::on_continueButton_clicked);

    gpgme_check_version(nullptr);
}

KeyDialog::~KeyDialog()
{
    delete ui;
}

void KeyDialog::on_continueButton_clicked()
{
    this->accept();
}

void KeyDialog::on_generateKeyButton_clicked()
{
    gpgme_ctx_t ctx;
    gpgme_error_t err;

    std::string userEmail = ui->emailLE->text().toStdString();
    ui->emailLE->clear();
    if (userEmail.empty()) {
        return;
    }

    std::string params = "<GnupgKeyParms format=\"internal\">\n"
                         "Key-Type: RSA\n"
                         "Key-Length: 4096\n"
                         "Subkey-Type: RSA\n"
                         "Subkey-Length: 4096\n"
                         "Name-Email: " + userEmail + "\n"
                         "Expire-Date: 0\n"
                         "</GnupgKeyParms>";
    const char *c_params = params.c_str();

    err = gpgme_new(&ctx);
    if (err)
    {
        QMessageBox::critical(nullptr, "Error", QString("Failed to initialize GPGME context: %1").arg(gpgme_strerror(err)));
        return;
    }

    err = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
    if (err)
    {
        QMessageBox::critical(nullptr, "Error", QString("Failed to set protocol as OpenPGP: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    err = gpgme_op_genkey(ctx, c_params, nullptr, nullptr);
    if (err)
    {
        QMessageBox::critical(nullptr, "Error", QString("Failed to generate key: %1").arg(gpgme_strerror(err)));
        gpgme_release(ctx);
        return;
    }

    gpgme_genkey_result_t res = gpgme_op_genkey_result(ctx);
    if (res->fpr) {
        QMessageBox::information(nullptr, "Success", QString("Key generated successfully!\nFingerprint: %1").arg(res->fpr));
    } else {
        QMessageBox::critical(nullptr, "Error", "Failed to generate key");
        gpgme_release(ctx);
        return;
    }

    gpgme_release(ctx);

    this->accept();
}
