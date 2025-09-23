#include "HardwareAccelerationService.h"
#include <QDebug>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QGuiApplication>
#include <QWindow>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QRegularExpression>

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <sys/utsname.h>
#endif

class HardwareAccelerationService::HardwareInfoWorker : public QObject
{
    Q_OBJECT

public slots:
    void loadHardwareInfo()
    {
        HardwareAccelerationInfo info;
        
        // Detect platform
        info.platform = getPlatformInfoInternal();
        
        // Get graphics card information
        info.graphicsCard = getGraphicsCardInfo();
        
        // Get OpenGL context information
        auto glInfo = getOpenGLInfo();
        info.openGLVersion = glInfo.openGLVersion;
        info.openGLVendor = glInfo.openGLVendor;
        info.openGLRenderer = glInfo.openGLRenderer;
        info.supportedExtensions = glInfo.supportedExtensions;
        info.qtRenderingBackend = glInfo.qtRenderingBackend;
        info.surfaceFormat = glInfo.surfaceFormat;
        
        // Determine if hardware accelerated
        info.isHardwareAccelerated = !info.openGLRenderer.toLower().contains("software") && 
                                   !info.openGLRenderer.toLower().contains("llvmpipe") &&
                                   !info.openGLRenderer.toLower().contains("mesa software");
        
        // Detect rendering backend
        info.renderingBackend = getRenderingBackend();
        
        // Check if software rendering is being used
        info.isSoftwareRendering = info.renderingBackend.contains("Software") || 
                                 info.openGLRenderer.toLower().contains("software") ||
                                 info.openGLRenderer.toLower().contains("llvmpipe") ||
                                 info.openGLRenderer.toLower().contains("mesa software");
        
        // Check EGL support on Linux
#ifdef Q_OS_LINUX
        info.isEglSupported = checkEglSupport();
#endif

        emit hardwareInfoReady(info);
    }

signals:
    void hardwareInfoReady(const HardwareAccelerationInfo& info);

private:
    QString getPlatformInfoInternal() const
    {
#ifdef Q_OS_WINDOWS
        return "Windows";
#elif defined(Q_OS_LINUX)
        return "Linux";
#elif defined(Q_OS_MACOS)
        return "macOS";
#else
        return "Unknown";
#endif
    }

    QString getGraphicsCardInfo() const
    {
#ifdef Q_OS_LINUX
        return getLinuxGraphicsInfo();
#elif defined(Q_OS_WINDOWS)
        return getWindowsGraphicsInfo();
#else
        return "Platform not supported for graphics detection";
#endif
    }

    QString getLinuxGraphicsInfo() const
    {
        // First, try alternative methods that don't require external commands
        QString graphicsInfo = getLinuxGraphicsInfoFromSysfs();
        if (!graphicsInfo.isEmpty())
        {
            return graphicsInfo;
        }

        // Try lspci only if available
        if (isCommandAvailable("lspci"))
        {
            QProcess process;
            process.start("lspci", QStringList() << "-nn");
            if (process.waitForFinished(3000))
            {
                QString output = process.readAllStandardOutput();
                QStringList lines = output.split('\n');
                for (const QString& line : lines)
                {
                    if (line.toLower().contains("vga") || line.toLower().contains("3d"))
                    {
                        return line.trimmed();
                    }
                }
            }
        }

        // Fallback methods
        if (QFile::exists("/proc/driver/nvidia/version"))
        {
            return "NVIDIA GPU (driver detected)";
        }
        
        // Check for AMD GPU
        if (QDir("/sys/class/drm").exists())
        {
            QDir drmDir("/sys/class/drm");
            QStringList cards = drmDir.entryList(QStringList() << "card*", QDir::Dirs);
            if (!cards.isEmpty())
            {
                return "GPU detected via DRM";
            }
        }

        return "Graphics hardware detection limited in container environment";
    }

    QString getLinuxGraphicsInfoFromSysfs() const
    {
        QDir drmDir("/sys/class/drm");
        if (!drmDir.exists())
            return "";

        QStringList cards = drmDir.entryList(QStringList() << "card*", QDir::Dirs);
        cards = cards.filter(QRegularExpression("^card\\d+$")); // Filter out card*-* entries

        if (cards.isEmpty())
            return "";

        QStringList gpuInfo;
        
        for (const QString& card : cards)
        {
            QString devicePath = drmDir.absoluteFilePath(card + "/device");
            
            // Try to read vendor and device info
            QString vendorFile = devicePath + "/vendor";
            QString deviceFile = devicePath + "/device";

            if (QFile::exists(vendorFile) && QFile::exists(deviceFile))
            {
                QFile vFile(vendorFile);
                QFile dFile(deviceFile);
                
                if (vFile.open(QIODevice::ReadOnly) && dFile.open(QIODevice::ReadOnly))
                {
                    QString vendor = vFile.readAll().trimmed();
                    QString device = dFile.readAll().trimmed();
                    
                    QString vendorName = getVendorName(vendor);
                    gpuInfo << QString("%1 GPU (%2) - Vendor: %3, Device: %4")
                                .arg(vendorName, card, vendor, device);
                }
            }
        }
        
        if (!gpuInfo.isEmpty())
        {
            return gpuInfo.join("; ");
        }
        
        return QString("Graphics devices detected: %1").arg(cards.join(", "));
    }

    QString getWindowsGraphicsInfo() const
    {
        return "Windows Graphics Device";
    }

    QString getRenderingBackend() const
    {
#ifdef Q_OS_LINUX
        // Check if we're running in a framebuffer environment
        QString displayEnv = qEnvironmentVariable("DISPLAY");
        QString waylandDisplay = qEnvironmentVariable("WAYLAND_DISPLAY");
        
        if (displayEnv.isEmpty() && waylandDisplay.isEmpty())
        {
            // Likely framebuffer mode
            if (QDir("/sys/class/graphics").exists())
            {
                QDir graphicsDir("/sys/class/graphics");
                QStringList frameBuffers = graphicsDir.entryList(QStringList() << "fb*", QDir::Dirs);
                if (!frameBuffers.isEmpty())
                {
                    return "Linux Framebuffer";
                }
            }
            return "Linux (No Display Server)";
        }
        else if (!waylandDisplay.isEmpty())
        {
            return "Wayland";
        }
        else if (!displayEnv.isEmpty())
        {
            return "X11";
        }
        
        return "Linux (Display Server Unknown)";
#elif defined(Q_OS_WINDOWS)
        return "Win32";
#elif defined(Q_OS_MACOS)
        return "macOS";
#else
        return "Unknown";
#endif
    }

    bool checkEglSupport() const
    {
        // Check if EGL libraries are available in common locations
        QStringList eglPaths = {
            "/usr/lib/x86_64-linux-gnu/libEGL.so.1",
            "/usr/lib/x86_64-linux-gnu/libEGL.so",
            "/usr/lib/aarch64-linux-gnu/libEGL.so.1",
            "/usr/lib/aarch64-linux-gnu/libEGL.so",
            "/usr/lib/arm-linux-gnueabihf/libEGL.so.1",
            "/usr/lib/arm-linux-gnueabihf/libEGL.so",
            "/usr/lib/libEGL.so.1",
            "/usr/lib/libEGL.so",
            "/usr/lib64/libEGL.so.1",
            "/usr/lib64/libEGL.so",
            "/lib/x86_64-linux-gnu/libEGL.so.1",
            "/lib/x86_64-linux-gnu/libEGL.so",
            "/lib/aarch64-linux-gnu/libEGL.so.1",
            "/lib/aarch64-linux-gnu/libEGL.so",
            "/opt/vc/lib/libEGL.so", // Raspberry Pi
            "/usr/local/lib/libEGL.so"
        };

        for (const QString& path : eglPaths)
        {
            if (QFile::exists(path))
            {
                qDebug() << "Found EGL library at:" << path;
                return true;
            }
        }

        // Check LD_LIBRARY_PATH for EGL libraries
        QString ldLibraryPath = qEnvironmentVariable("LD_LIBRARY_PATH");
        if (!ldLibraryPath.isEmpty())
        {
            QStringList paths = ldLibraryPath.split(':');
            for (const QString& path : paths)
            {
                QDir dir(path);
                if (dir.exists())
                {
                    QStringList eglFiles = dir.entryList(QStringList() << "libEGL.so*", QDir::Files);
                    if (!eglFiles.isEmpty())
                    {
                        qDebug() << "Found EGL library in LD_LIBRARY_PATH:" << dir.absoluteFilePath(eglFiles.first());
                        return true;
                    }
                }
            }
        }

        // Try to run eglinfo if available (but don't fail if missing)
        if (isCommandAvailable("eglinfo"))
        {
            QProcess process;
            process.start("eglinfo");
            if (process.waitForFinished(3000))
            {
                bool success = (process.exitCode() == 0);
                if (success)
                {
                    qDebug() << "eglinfo command executed successfully";
                }
                return success;
            }
        }

        return false;
    }

    bool isCommandAvailable(const QString& command) const
    {
        QProcess process;
        process.start("which", QStringList() << command);
        if (process.waitForFinished(1000))
        {
            return process.exitCode() == 0;
        }
        
        // Alternative approach
        process.start(command, QStringList() << "--version");
        if (process.waitForFinished(1000))
        {
            return process.exitCode() == 0;
        }

        return false;
    }

    QString getVendorName(const QString& vendorId) const
    {
        QString vid = vendorId.toLower();
        if (vid == "0x1002") return "AMD";
        if (vid == "0x10de") return "NVIDIA";
        if (vid == "0x8086") return "Intel";
        if (vid == "0x1234") return "QEMU";
        if (vid == "0x15ad") return "VMware";
        return vendorId;
    }

    HardwareAccelerationInfo getOpenGLInfo() const
    {
        HardwareAccelerationInfo info;
        
        QOffscreenSurface surface;
        surface.create();
        
        QOpenGLContext context;
        if (context.create() && context.makeCurrent(&surface))
        {
            QOpenGLFunctions* gl = context.functions();
            if (gl)
            {
                const GLubyte* version = gl->glGetString(GL_VERSION);
                const GLubyte* vendor = gl->glGetString(GL_VENDOR);
                const GLubyte* renderer = gl->glGetString(GL_RENDERER);
                
                if (version)
                    info.openGLVersion = QString::fromLatin1(reinterpret_cast<const char*>(version));
                if (vendor)
                    info.openGLVendor = QString::fromLatin1(reinterpret_cast<const char*>(vendor));
                if (renderer)
                    info.openGLRenderer = QString::fromLatin1(reinterpret_cast<const char*>(renderer));
                
                // Get extensions using the older method for compatibility
                const GLubyte* extensionsString = gl->glGetString(GL_EXTENSIONS);
                if (extensionsString)
                {
                    QString extensions = QString::fromLatin1(reinterpret_cast<const char*>(extensionsString));
                    QStringList extensionList = extensions.split(' ', Qt::SkipEmptyParts);
                    
                    // Limit to first 20 extensions to avoid excessive data
                    int maxExtensions = qMin(extensionList.size(), 20);
                    for (int i = 0; i < maxExtensions; ++i)
                    {
                        info.supportedExtensions << extensionList[i];
                    }
                    
                    if (extensionList.size() > 20)
                    {
                        info.supportedExtensions << QString("... and %1 more extensions").arg(extensionList.size() - 20);
                    }
                }
            }
            
            // Get Qt rendering backend info
            QSurfaceFormat format = context.format();
            info.qtRenderingBackend = QString("OpenGL %1.%2").arg(format.majorVersion()).arg(format.minorVersion());
            
            if (format.renderableType() == QSurfaceFormat::OpenGL)
                info.qtRenderingBackend += " (OpenGL)";
            else if (format.renderableType() == QSurfaceFormat::OpenGLES)
                info.qtRenderingBackend += " (OpenGL ES)";
            
            info.surfaceFormat = QString("Color: %1, Depth: %2, Stencil: %3, Samples: %4")
                                    .arg(format.redBufferSize() + format.greenBufferSize() + format.blueBufferSize())
                                    .arg(format.depthBufferSize())
                                    .arg(format.stencilBufferSize())
                                    .arg(format.samples());
            
            context.doneCurrent();
        }
        else
        {
            info.openGLVersion = "Failed to create OpenGL context";
            info.openGLVendor = "Unknown";
            info.openGLRenderer = "Unknown";
            info.qtRenderingBackend = "Failed to initialize";
            info.surfaceFormat = "Not available";
        }
        
        return info;
    }
};

HardwareAccelerationService::HardwareAccelerationService(QObject *parent)
    : QObject(parent)
    , m_hardwareInfoText("Loading hardware information...")
    , m_detailedReportText("")
    , m_isLoading(true)
{
    loadHardwareInfoAsync();
}

void HardwareAccelerationService::refreshHardwareInfo()
{
    m_isLoading = true;
    emit loadingChanged();
    loadHardwareInfoAsync();
}

QString HardwareAccelerationService::getPlatformInfo() const
{
#ifdef Q_OS_WINDOWS
    return "Windows";
#elif defined(Q_OS_LINUX)
    return "Linux";
#elif defined(Q_OS_MACOS)
    return "macOS";
#else
    return "Unknown";
#endif
}

void HardwareAccelerationService::loadHardwareInfoAsync()
{
    HardwareInfoWorker* worker = new HardwareInfoWorker();
    QThread* thread = new QThread(this);
    worker->moveToThread(thread);
    
    connect(thread, &QThread::started, worker, &HardwareInfoWorker::loadHardwareInfo);
    connect(worker, &HardwareInfoWorker::hardwareInfoReady, this, [this](const HardwareAccelerationInfo& info) {
        m_info = info;
        m_hardwareInfoText = formatHardwareInfoText(m_info);
        m_detailedReportText = formatDetailedReport(m_info);
        m_isLoading = false;
        emit loadingChanged();
        emit hardwareInfoChanged();
    });
    connect(worker, &HardwareInfoWorker::hardwareInfoReady, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    
    thread->start();
}

void HardwareAccelerationService::onHardwareInfoReady()
{
    // This method is no longer needed as we handle it in the lambda above
}

HardwareAccelerationInfo HardwareAccelerationService::getHardwareAccelerationInfo()
{
    HardwareAccelerationInfo info;
    
    // This is a simplified synchronous version for the main thread
    info.platform = getPlatformInfo();
    info.graphicsCard = "Use async loading for detailed info";
    info.renderingBackend = "Qt/QML";
    
    // Try to get basic OpenGL info
    QOffscreenSurface surface;
    surface.create();
    
    QOpenGLContext context;
    if (context.create() && context.makeCurrent(&surface))
    {
        QOpenGLFunctions* gl = context.functions();
        if (gl)
        {
            const GLubyte* version = gl->glGetString(GL_VERSION);
            const GLubyte* vendor = gl->glGetString(GL_VENDOR);
            const GLubyte* renderer = gl->glGetString(GL_RENDERER);
            
            if (version)
                info.openGLVersion = QString::fromLatin1(reinterpret_cast<const char*>(version));
            if (vendor)
                info.openGLVendor = QString::fromLatin1(reinterpret_cast<const char*>(vendor));
            if (renderer)
                info.openGLRenderer = QString::fromLatin1(reinterpret_cast<const char*>(renderer));
            
            info.isHardwareAccelerated = !info.openGLRenderer.toLower().contains("software") && 
                                       !info.openGLRenderer.toLower().contains("llvmpipe");
            info.isSoftwareRendering = !info.isHardwareAccelerated;
        }
        context.doneCurrent();
    }
    
    return info;
}

QString HardwareAccelerationService::formatHardwareInfoText(const HardwareAccelerationInfo& info) const
{
    return QString("Platform: %1\n"
                  "Hardware Accelerated: %2\n"
                  "Rendering Backend: %3\n"
                  "Graphics Card: %4\n"
                  "OpenGL Renderer: %5")
            .arg(info.platform)
            .arg(info.isHardwareAccelerated ? "✅ Yes" : "❌ No")
            .arg(info.renderingBackend)
            .arg(info.graphicsCard)
            .arg(info.openGLRenderer);
}

QString HardwareAccelerationService::formatDetailedReport(const HardwareAccelerationInfo& info) const
{
    QString report = "=== Hardware Acceleration Report ===\n";
    report += QString("Platform: %1\n").arg(info.platform);
    report += QString("Hardware Accelerated: %1\n").arg(info.isHardwareAccelerated ? "Yes" : "No");
    report += QString("Software Rendering: %1\n").arg(info.isSoftwareRendering ? "Yes" : "No");
    report += QString("Rendering Backend: %1\n").arg(info.renderingBackend);
    report += QString("Graphics Card: %1\n").arg(info.graphicsCard);
    report += QString("OpenGL Version: %1\n").arg(info.openGLVersion);
    report += QString("OpenGL Vendor: %1\n").arg(info.openGLVendor);
    report += QString("OpenGL Renderer: %1\n").arg(info.openGLRenderer);
    
    if (!info.qtRenderingBackend.isEmpty())
    {
        report += QString("Qt Rendering Backend: %1\n").arg(info.qtRenderingBackend);
    }
    
    if (!info.surfaceFormat.isEmpty())
    {
        report += QString("Surface Format: %1\n").arg(info.surfaceFormat);
    }
    
#ifdef Q_OS_LINUX
    report += QString("EGL Support: %1\n").arg(info.isEglSupported ? "Yes" : "No");
#endif
    
    if (!info.supportedExtensions.isEmpty() && info.supportedExtensions.size() < 50)
    {
        report += QString("OpenGL Extensions: %1\n").arg(info.supportedExtensions.join(", "));
    }
    else if (!info.supportedExtensions.isEmpty())
    {
        report += QString("OpenGL Extensions: %1 extensions available\n").arg(info.supportedExtensions.size());
    }
    
    return report;
}

#include "HardwareAccelerationService.moc"