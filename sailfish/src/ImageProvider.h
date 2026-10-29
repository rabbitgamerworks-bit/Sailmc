#pragma once

#include <QQuickImageProvider>

/*
 * image://sailmc/instance/<iconKey>  - instance icon from the launcher IconList
 * image://sailmc/face/<accountId>    - Minecraft skin face of an account
 * image://sailmc/qr/<text>           - QR code (used for the Microsoft login link)
 */
class ImageProvider : public QQuickImageProvider
{
public:
    ImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};
