#ifndef HARDWAREACCELERATIONSERVICE_H
#define HARDWAREACCELERATIONSERVICE_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

#if __has_include(<GL/gl.h>)
#include <GL/gl.h>
#define HAS_OPENGL 1
#else
#define HAS_OPENGL 0
#endif

#if __has_include(<EGL/egl.h>)
#include <EGL/egl.h>
#define HAS_EGL 1
#else
#define HAS_EGL 0
#endif

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
    using UpdateCallback = std::function<void(const HardwareAccelerationInfo&)>;
    
    HardwareAccelerationService();
    ~HardwareAccelerationService();
    
    // Getters
    bool isHardwareAccelerated() const;
    std::string getHardwareInfoText() const;
    std::string getDetailedReport() const;
    bool isLoading() const;
    
    // Methods
    void refreshHardwareInfo();
    void refreshHardwareInfoAsync(UpdateCallback callback);
    std::string getPlatformInfo() const;
    
    // Get current info
    const HardwareAccelerationInfo& getInfo() const { return m_info; }

private:
    void loadHardwareInfo();
    HardwareAccelerationInfo getHardwareAccelerationInfo();
    std::string getPlatformInfoInternal() const;
    std::string getGraphicsCardInfo() const;
    std::string getLinuxGraphicsInfo() const;
    std::string getLinuxGraphicsInfoFromSysfs() const;
    std::string getRenderingBackend() const;
    bool checkEglSupport() const;
    bool isCommandAvailable(const std::string& command) const;
    std::string getVendorName(const std::string& vendorId) const;
    HardwareAccelerationInfo getOpenGLInfo() const;
    std::string formatDetailedReport(const HardwareAccelerationInfo& info) const;
    std::string formatHardwareInfoText(const HardwareAccelerationInfo& info) const;

    HardwareAccelerationInfo m_info;
    std::string m_hardwareInfoText;
    std::string m_detailedReportText;
    std::atomic<bool> m_isLoading{false};
    
    mutable std::mutex m_mutex;
    std::unique_ptr<std::thread> m_workerThread;
    UpdateCallback m_updateCallback;
};

#endif // HARDWAREACCELERATIONSERVICE_H