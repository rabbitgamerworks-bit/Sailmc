#include "ImageProvider.h"

#include <QIcon>
#include <QUrl>

#include <qrcode/QrCodeGenerator.h>

#include "Application.h"
#include "icons/IconList.h"
#include "models/AccountModel.h"

ImageProvider::ImageProvider() : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage ImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const int slash = id.indexOf('/');
    const QString kind = id.left(slash);
    const QString arg = QUrl::fromPercentEncoding(id.mid(slash + 1).toUtf8());
    const QSize wanted = requestedSize.isValid() ? requestedSize : QSize(128, 128);

    QImage image;
    if (kind == "instance") {
        auto icon = APPLICATION->icons()->getIcon(arg.isEmpty() ? QString("default") : arg);
        image = icon.pixmap(wanted).toImage();
        if (!image.isNull() && image.size() != wanted) {
            image = image.scaled(wanted, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    } else if (kind == "face") {
        auto account = AccountModel::byId(arg);
        if (account) {
            // the face is 8x8 pixel art, keep it crisp
            image = account->getFace().toImage().scaled(wanted, Qt::KeepAspectRatio, Qt::FastTransformation);
        }
    } else if (kind == "qr") {
        image = qrcode::generateQr(arg, qMax(wanted.width(), 64));
    }

    if (size) {
        *size = image.size();
    }
    return image;
}
