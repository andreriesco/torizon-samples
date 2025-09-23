#ifndef HARDWAREACCELERATIONSERVICE_H
#define HARDWAREACCELERATIONSERVICE_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

struct HardwareAccelerationInfo {
    bool isHardwareAccelerated = false;
    std::string renderingBackend = "Unknown";
    std::string openGLVersion = "Unknown";
    std::string openGLVendor = "Unknown";
    std::string openGLRenderer = "Unknown";
    std::string graphicsCard = "Unknown";
    std::vector<std::string> supportedExtensions;
    bool isEglSupported = false;
    bool isSoftwareRendering = false;
    std::string platform = "Unknown";
    std::string surfaceFormat = "Unknown";
};

class HardwareAccelerationService {
public:
    using InfoCallback = std::function<void(const HardwareAccelerationInfo&)>;

    HardwareAccelerationService();
    ~HardwareAccelerationService() = default;

    // Synchronous methods for immediate access
    HardwareAccelerationInfo getHardwareAccelerationInfo();
    std::string getPlatformInfo() const;
    std::string getHardwareInfoText(const HardwareAccelerationInfo& info) const;
    std::string getDetailedReport(const HardwareAccelerationInfo& info) const;

    // Asynchronous method for non-blocking operation
    void loadHardwareInfoAsync(InfoCallback callback);

private:
    std::string getPlatformInfoInternal() const;
    std::string getGraphicsCardInfo() const;
    std::string getLinuxGraphicsInfo() const;
    std::string getLinuxGraphicsInfoFromSysfs() const;
    std::string getWindowsGraphicsInfo() const;
    std::string getRenderingBackend() const;
    bool checkEglSupport() const;
    bool isCommandAvailable(const std::string& command) const;
    std::string getVendorName(const std::string& vendorId) const;
    HardwareAccelerationInfo getOpenGLInfo() const;
    
    HardwareAccelerationInfo m_cachedInfo;
    bool m_infoLoaded = false;
};

#endif // HARDWAREACCELERATIONSERVICE_H