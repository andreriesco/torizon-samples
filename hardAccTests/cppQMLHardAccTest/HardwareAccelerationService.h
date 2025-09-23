#ifndef HARDWAREACCELERATIONSERVICE_H
#define HARDWAREACCELERATIONSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QThread>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>

struct HardwareAccelerationInfo {
    bool isHardwareAccelerated = false;
    QString renderingBackend = "Unknown";
    QString openGLVersion = "Unknown";
    QString openGLVendor = "Unknown";
    QString openGLRenderer = "Unknown";
    QString graphicsCard = "Unknown";
    QStringList supportedExtensions;
    bool isEglSupported = false;
    bool isSoftwareRendering = false;
    QString platform = "Unknown";
    QString qtRenderingBackend = "Unknown";
    QString surfaceFormat = "Unknown";
};

class HardwareAccelerationService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isHardwareAccelerated READ isHardwareAccelerated NOTIFY hardwareInfoChanged)
    Q_PROPERTY(QString hardwareInfo READ hardwareInfo NOTIFY hardwareInfoChanged)
    Q_PROPERTY(QString detailedReport READ detailedReport NOTIFY hardwareInfoChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)

public:
    explicit HardwareAccelerationService(QObject *parent = nullptr);
    
    bool isHardwareAccelerated() const { return m_info.isHardwareAccelerated; }
    QString hardwareInfo() const { return m_hardwareInfoText; }
    QString detailedReport() const { return m_detailedReportText; }
    bool isLoading() const { return m_isLoading; }
    
    Q_INVOKABLE void refreshHardwareInfo();
    Q_INVOKABLE QString getPlatformInfo() const;

signals:
    void hardwareInfoChanged();
    void loadingChanged();

private slots:
    void onHardwareInfoReady();

private:
    void loadHardwareInfoAsync();
    HardwareAccelerationInfo getHardwareAccelerationInfo();
    QString getPlatformInfoInternal() const;
    QString getGraphicsCardInfo() const;
    QString getLinuxGraphicsInfo() const;
    QString getLinuxGraphicsInfoFromSysfs() const;
    QString getWindowsGraphicsInfo() const;
    QString getRenderingBackend() const;
    bool checkEglSupport() const;
    bool isCommandAvailable(const QString& command) const;
    QString getVendorName(const QString& vendorId) const;
    HardwareAccelerationInfo getOpenGLInfo() const;
    QString formatDetailedReport(const HardwareAccelerationInfo& info) const;
    QString formatHardwareInfoText(const HardwareAccelerationInfo& info) const;

    HardwareAccelerationInfo m_info;
    QString m_hardwareInfoText;
    QString m_detailedReportText;
    bool m_isLoading = false;
    
    class HardwareInfoWorker;
    friend class HardwareInfoWorker;
};

#endif // HARDWAREACCELERATIONSERVICE_H