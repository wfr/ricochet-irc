/* Ricochet - https://ricochet.im/
 * Copyright (C) 2014, John Brooks <john.brooks@dereferenced.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the names of the copyright owners nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "main-shared.h"
#include "ui/MainWindow.h"
#include "core/IdentityManager.h"
#include "tor/TorManager.h"
#include "tor/TorControl.h"
#include "utils/CryptoKey.h"
#include "utils/SecureRNG.h"
#include "utils/Settings.h"
#include <QApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QSettings>
#include <QTime>
#include <QDir>
#include <QTranslator>
#include <QMessageBox>
#include <QLocale>
#include <QLockFile>
#include <QStandardPaths>
#include <openssl/crypto.h>


int main(int argc, char *argv[])
{
    /* Disable rwx memory.
       This will also ensure full PAX/Grsecurity protections. */
    qputenv("QV4_FORCE_INTERPRETER",  "1");
    qputenv("QT_ENABLE_REGEXP_JIT",   "0");

    QApplication a(argc, argv);
    a.setApplicationVersion(QLatin1String("1.1.2"));
    a.setOrganizationName(QStringLiteral("Ricochet"));

#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
    a.setWindowIcon(QIcon(QStringLiteral(":/icons/ricochet.svg")));
#endif

    QScopedPointer<SettingsFile> settings(new SettingsFile);
    SettingsObject::setDefaultFile(settings.data());

    QString error;
    QLockFile *lock = 0;
    if (!initSettings(settings.data(), &lock, error)) {
        QMessageBox::critical(0, qApp->translate("Main", "Ricochet Error"), error);
        return 1;
    }
    QScopedPointer<QLockFile> lockFile(lock);

    initTranslation();

    /* Initialize OpenSSL's allocator */
    CRYPTO_malloc_init();

    /* Seed the OpenSSL RNG */
    if (!SecureRNG::seed())
        qFatal("Failed to initialize RNG");
    qsrand(SecureRNG::randomInt(UINT_MAX));

    /* Tor control manager */
    Tor::TorManager *torManager = Tor::TorManager::instance();
    torManager->setDataDirectory(QFileInfo(settings->filePath()).path() + QStringLiteral("/tor/"));
    torControl = torManager->control();
    torManager->start();

    /* Identities */
    identityManager = new IdentityManager;
    QScopedPointer<IdentityManager> scopedIdentityManager(identityManager);

    /* Window */
    QScopedPointer<MainWindow> w(new MainWindow);
    if (!w->showUI())
        return 1;

    return a.exec();
}
