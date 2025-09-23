#include "../includes/HardwareAccelerationService.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <regex>
#include <algorithm>
#include <unistd.h>
#include <sys/utsname.h>

HardwareAccelerationService::HardwareAccelerationService() {
    // Load initial hardware info synchronously
    loadHardwareInfo();
}

HardwareAccelerationService::~HardwareAccelerationService() {
    if (m_workerThread && m_workerThread->joinable()) {
        m_workerThread->join();
    }
}

bool HardwareAccelerationService::isHardwareAccelerated() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_info.isHardwareAccelerated;
}

std::string HardwareAccelerationService::getHardwareInfoText() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hardwareInfoText;
}

std::string HardwareAccelerationService::getDetailedReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_detailedReportText;
}

bool HardwareAccelerationService::isLoading() const {
    return m_isLoading.load();
}

void HardwareAccelerationService::refreshHardwareInfo() {
    loadHardwareInfo();
}

void HardwareAccelerationService::refreshHardwareInfoAsync(UpdateCallback callback) {
    if (m_workerThread && m_workerThread->joinable()) {
        m_workerThread->join();
    }
    
    m_updateCallback = callback;
    m_isLoading = true;
    
    m_workerThread = std::make_unique<std::thread>([this]() {
        loadHardwareInfo();
        m_isLoading = false;
        if (m_updateCallback) {
            m_updateCallback(m_info);
        }
    });
}

std::string HardwareAccelerationService::getPlatformInfo() const {
    return getPlatformInfoInternal();
}

void HardwareAccelerationService::loadHardwareInfo() {
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
    info.surfaceFormat = glInfo.surfaceFormat;
    
    // Determine if hardware accelerated
    std::string renderer = info.openGLRenderer;
    std::transform(renderer.begin(), renderer.end(), renderer.begin(), ::tolower);
    info.isHardwareAccelerated = renderer.find("software") == std::string::npos && 
                               renderer.find("llvmpipe") == std::string::npos &&
                               renderer.find("mesa software") == std::string::npos;
    
    // Determine if software rendering
    info.isSoftwareRendering = !info.isHardwareAccelerated;
    
    // Get rendering backend
    info.renderingBackend = getRenderingBackend();
    
    // Check EGL support
    info.isEglSupported = checkEglSupport();
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_info = info;
        m_hardwareInfoText = formatHardwareInfoText(info);
        m_detailedReportText = formatDetailedReport(info);
    }
}

HardwareAccelerationInfo HardwareAccelerationService::getHardwareAccelerationInfo() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_info;
}

std::string HardwareAccelerationService::getPlatformInfoInternal() const {
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        std::ostringstream oss;
        oss << unameData.sysname << " " << unameData.release << " " << unameData.machine;
        return oss.str();
    }
    return "Linux (unknown version)";
}

std::string HardwareAccelerationService::getGraphicsCardInfo() const {
    // First try sysfs approach
    std::string sysfsInfo = getLinuxGraphicsInfoFromSysfs();
    if (!sysfsInfo.empty()) {
        return sysfsInfo;
    }
    
    // Fallback to lspci if available
    return getLinuxGraphicsInfo();
}

std::string HardwareAccelerationService::getLinuxGraphicsInfo() const {
    try {
        // Try lspci if available
        if (isCommandAvailable("lspci")) {
            std::string command = "lspci -nn 2>/dev/null | grep -i 'vga\\|3d\\|display'";
            FILE* pipe = popen(command.c_str(), "r");
            if (pipe) {
                char buffer[256];
                std::string result;
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    result += buffer;
                }
                pclose(pipe);
                
                if (!result.empty()) {
                    // Remove trailing newline
                    if (result.back() == '\n') {
                        result.pop_back();
                    }
                    return result;
                }
            }
        }
        
        // Check for NVIDIA driver
        if (std::filesystem::exists("/proc/driver/nvidia/version")) {
            return "NVIDIA GPU (driver detected)";
        }
        
        // Check for AMD GPU via DRM
        if (std::filesystem::exists("/sys/class/drm")) {
            for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm")) {
                if (entry.is_directory() && entry.path().filename().string().find("card") == 0) {
                    return "GPU detected via DRM";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting Linux graphics info: " << e.what() << std::endl;
    }
    
    return "Graphics hardware detection limited in container environment";
}

std::string HardwareAccelerationService::getLinuxGraphicsInfoFromSysfs() const {
    try {
        if (!std::filesystem::exists("/sys/class/drm")) {
            return "";
        }
        
        std::vector<std::string> cards;
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm")) {
            if (entry.is_directory()) {
                std::string cardName = entry.path().filename().string();
                // Filter out card*-* entries, keep only card[0-9]+
                if (std::regex_match(cardName, std::regex("^card\\d+$"))) {
                    cards.push_back(cardName);
                }
            }
        }
        
        if (cards.empty()) {
            return "";
        }
        
        std::vector<std::string> gpuInfo;
        
        for (const auto& card : cards) {
            std::string devicePath = "/sys/class/drm/" + card + "/device";
            
            // Try to read vendor and device info
            std::string vendorFile = devicePath + "/vendor";
            std::string deviceFile = devicePath + "/device";
            
            if (std::filesystem::exists(vendorFile) && std::filesystem::exists(deviceFile)) {
                try {
                    std::ifstream vf(vendorFile);
                    std::ifstream df(deviceFile);
                    std::string vendor, device;
                    
                    if (vf >> vendor && df >> device) {
                        std::string vendorName = getVendorName(vendor);
                        gpuInfo.push_back(vendorName + " GPU (" + card + ") - Vendor: " + vendor + ", Device: " + device);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error reading vendor/device info for " << card << ": " << e.what() << std::endl;
                }
            }
        }
        
        if (!gpuInfo.empty()) {
            std::string result;
            for (size_t i = 0; i < gpuInfo.size(); ++i) {
                if (i > 0) result += "; ";
                result += gpuInfo[i];
            }
            return result;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting Linux graphics info from sysfs: " << e.what() << std::endl;
    }
    
    return "";
}

std::string HardwareAccelerationService::getRenderingBackend() const {
    return "GTK3";
}

bool HardwareAccelerationService::checkEglSupport() const {
#if HAS_EGL
    // Check if EGL libraries are available in common locations
    std::vector<std::string> eglPaths = {
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
        "/opt/vc/lib/libEGL.so",  // Raspberry Pi
        "/usr/local/lib/libEGL.so"
    };
    
    for (const auto& path : eglPaths) {
        if (std::filesystem::exists(path)) {
            std::cout << "Found EGL library at: " << path << std::endl;
            return true;
        }
    }
    
    // Check LD_LIBRARY_PATH for EGL libraries
    const char* ldLibraryPath = std::getenv("LD_LIBRARY_PATH");
    if (ldLibraryPath && strlen(ldLibraryPath) > 0) {
        std::string paths = ldLibraryPath;
        std::istringstream ss(paths);
        std::string path;
        
        while (std::getline(ss, path, ':')) {
            try {
                for (const auto& entry : std::filesystem::directory_iterator(path)) {
                    if (entry.is_regular_file()) {
                        std::string filename = entry.path().filename().string();
                        if (filename.find("libEGL.so") != std::string::npos) {
                            std::cout << "Found EGL library in LD_LIBRARY_PATH: " << entry.path() << std::endl;
                            return true;
                        }
                    }
                }
            } catch (const std::exception& e) {
                // Ignore errors for invalid paths
            }
        }
    }
    
    // Try to run eglinfo if available (but don't fail if missing)
    if (isCommandAvailable("eglinfo")) {
        try {
            int result = std::system("eglinfo >/dev/null 2>&1");
            if (result == 0) {
                std::cout << "eglinfo command executed successfully" << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error running eglinfo: " << e.what() << std::endl;
        }
    }
#endif
    
    return false;
}

bool HardwareAccelerationService::isCommandAvailable(const std::string& command) const {
    std::string cmd = "which " + command + " >/dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}

std::string HardwareAccelerationService::getVendorName(const std::string& vendorId) const {
    std::string vid = vendorId;
    std::transform(vid.begin(), vid.end(), vid.begin(), ::tolower);
    
    if (vid == "0x1002") return "AMD";
    if (vid == "0x10de") return "NVIDIA";
    if (vid == "0x8086") return "Intel";
    if (vid == "0x1234") return "QEMU";
    if (vid == "0x15ad") return "VMware";
    
    return vendorId;
}

HardwareAccelerationInfo HardwareAccelerationService::getOpenGLInfo() const {
    HardwareAccelerationInfo result;
    result.openGLVersion = "OpenGL detection requires runtime context";
    result.openGLVendor = "Unknown";
    result.openGLRenderer = "Unknown";
    result.surfaceFormat = "Unknown";
    
#if HAS_OPENGL
    // On Linux, we can try to get some basic GL info from glxinfo if available
    if (isCommandAvailable("glxinfo")) {
        try {
            FILE* pipe = popen("glxinfo 2>/dev/null", "r");
            if (pipe) {
                char buffer[256];
                std::string output;
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                pclose(pipe);
                
                std::istringstream ss(output);
                std::string line;
                while (std::getline(ss, line)) {
                    if (line.find("OpenGL vendor string:") != std::string::npos) {
                        size_t pos = line.find(':');
                        if (pos != std::string::npos && pos + 1 < line.length()) {
                            result.openGLVendor = line.substr(pos + 1);
                            // Trim whitespace
                            result.openGLVendor.erase(0, result.openGLVendor.find_first_not_of(" \t"));
                            result.openGLVendor.erase(result.openGLVendor.find_last_not_of(" \t") + 1);
                        }
                    } else if (line.find("OpenGL renderer string:") != std::string::npos) {
                        size_t pos = line.find(':');
                        if (pos != std::string::npos && pos + 1 < line.length()) {
                            result.openGLRenderer = line.substr(pos + 1);
                            // Trim whitespace
                            result.openGLRenderer.erase(0, result.openGLRenderer.find_first_not_of(" \t"));
                            result.openGLRenderer.erase(result.openGLRenderer.find_last_not_of(" \t") + 1);
                        }
                    } else if (line.find("OpenGL version string:") != std::string::npos) {
                        size_t pos = line.find(':');
                        if (pos != std::string::npos && pos + 1 < line.length()) {
                            result.openGLVersion = line.substr(pos + 1);
                            // Trim whitespace
                            result.openGLVersion.erase(0, result.openGLVersion.find_first_not_of(" \t"));
                            result.openGLVersion.erase(result.openGLVersion.find_last_not_of(" \t") + 1);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error running glxinfo: " << e.what() << std::endl;
        }
    }
    
    // If glxinfo didn't work, try alternative methods
    if (result.openGLRenderer == "Unknown") {
        // Check if we're in a software rendering environment
        if (std::filesystem::exists("/usr/lib/x86_64-linux-gnu/dri/swrast_dri.so") ||
            std::filesystem::exists("/usr/lib/dri/swrast_dri.so")) {
            result.openGLRenderer = "Software Rasterizer (Mesa)";
        }
    }
#endif
    
    return result;
}

std::string HardwareAccelerationService::formatDetailedReport(const HardwareAccelerationInfo& info) const {
    std::ostringstream oss;
    oss << "=== Hardware Acceleration Report ===" << std::endl;
    oss << "Platform: " << info.platform << std::endl;
    oss << "Hardware Accelerated: " << (info.isHardwareAccelerated ? "Yes" : "No") << std::endl;
    oss << "Software Rendering: " << (info.isSoftwareRendering ? "Yes" : "No") << std::endl;
    oss << "Rendering Backend: " << info.renderingBackend << std::endl;
    oss << "Graphics Card: " << info.graphicsCard << std::endl;
    oss << "OpenGL Version: " << info.openGLVersion << std::endl;
    oss << "OpenGL Vendor: " << info.openGLVendor << std::endl;
    oss << "OpenGL Renderer: " << info.openGLRenderer << std::endl;
    
    if (!info.surfaceFormat.empty() && info.surfaceFormat != "Unknown") {
        oss << "Surface Format: " << info.surfaceFormat << std::endl;
    }
    
    oss << "EGL Support: " << (info.isEglSupported ? "Yes" : "No") << std::endl;
    
    if (!info.supportedExtensions.empty()) {
        oss << "Supported Extensions: ";
        for (size_t i = 0; i < info.supportedExtensions.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << info.supportedExtensions[i];
        }
        oss << std::endl;
    }
    
    return oss.str();
}

std::string HardwareAccelerationService::formatHardwareInfoText(const HardwareAccelerationInfo& info) const {
    std::ostringstream oss;
    oss << "Platform: " << info.platform << std::endl;
    oss << "Hardware Accelerated: " << (info.isHardwareAccelerated ? "✅ Yes" : "❌ No") << std::endl;
    oss << "Rendering Backend: " << info.renderingBackend << std::endl;
    oss << "Graphics Card: " << info.graphicsCard << std::endl;
    oss << "OpenGL Renderer: " << info.openGLRenderer;
    
    return oss.str();
}