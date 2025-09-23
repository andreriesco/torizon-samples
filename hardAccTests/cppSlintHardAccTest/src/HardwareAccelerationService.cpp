#include "HardwareAccelerationService.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <thread>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#ifdef __linux__
#include <unistd.h>
#include <sys/utsname.h>
#include <dlfcn.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

// Try to include OpenGL headers if available, but don't fail if missing
#ifdef __linux__
// Only include if available, otherwise we'll work without them
#if __has_include(<GL/gl.h>)
#include <GL/gl.h>
#define HAS_GL_HEADERS 1
#endif

#if __has_include(<EGL/egl.h>)
#include <EGL/egl.h>
#define HAS_EGL_HEADERS 1
#endif

#if __has_include(<X11/Xlib.h>)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#define HAS_X11_HEADERS 1
#endif
#endif

HardwareAccelerationService::HardwareAccelerationService() = default;

HardwareAccelerationInfo HardwareAccelerationService::getHardwareAccelerationInfo() {
    if (m_infoLoaded) {
        return m_cachedInfo;
    }

    HardwareAccelerationInfo info;
    
    // Detect platform
    info.platform = getPlatformInfoInternal();
    
    // Get graphics card information
    info.graphicsCard = getGraphicsCardInfo();
    
    // Get OpenGL context information (basic)
    auto glInfo = getOpenGLInfo();
    info.openGLVersion = glInfo.openGLVersion;
    info.openGLVendor = glInfo.openGLVendor;
    info.openGLRenderer = glInfo.openGLRenderer;
    info.supportedExtensions = glInfo.supportedExtensions;
    info.surfaceFormat = glInfo.surfaceFormat;
    
    // Determine if hardware accelerated
    std::string rendererLower = info.openGLRenderer;
    std::transform(rendererLower.begin(), rendererLower.end(), rendererLower.begin(), ::tolower);
    
    info.isHardwareAccelerated = rendererLower.find("software") == std::string::npos && 
                               rendererLower.find("llvmpipe") == std::string::npos &&
                               rendererLower.find("mesa software") == std::string::npos;
    
    // Detect rendering backend
    info.renderingBackend = getRenderingBackend();
    
    // Check if software rendering is being used
    info.isSoftwareRendering = info.renderingBackend.find("Software") != std::string::npos || 
                             rendererLower.find("software") != std::string::npos ||
                             rendererLower.find("llvmpipe") != std::string::npos ||
                             rendererLower.find("mesa software") != std::string::npos;
    
    // Check EGL support on Linux
#ifdef __linux__
    info.isEglSupported = checkEglSupport();
#endif

    m_cachedInfo = info;
    m_infoLoaded = true;
    return info;
}

std::string HardwareAccelerationService::getPlatformInfo() const {
    return getPlatformInfoInternal();
}

std::string HardwareAccelerationService::getHardwareInfoText(const HardwareAccelerationInfo& info) const {
    std::ostringstream oss;
    oss << "Platform: " << info.platform << "\n"
        << "Hardware Accelerated: " << (info.isHardwareAccelerated ? "✅ Yes" : "❌ No") << "\n"
        << "Rendering Backend: " << info.renderingBackend << "\n"
        << "Graphics Card: " << info.graphicsCard << "\n"
        << "OpenGL Renderer: " << info.openGLRenderer;
    return oss.str();
}

std::string HardwareAccelerationService::getDetailedReport(const HardwareAccelerationInfo& info) const {
    std::ostringstream oss;
    oss << "=== Hardware Acceleration Report ===\n"
        << "Platform: " << info.platform << "\n"
        << "Hardware Accelerated: " << (info.isHardwareAccelerated ? "Yes" : "No") << "\n"
        << "Software Rendering: " << (info.isSoftwareRendering ? "Yes" : "No") << "\n"
        << "Rendering Backend: " << info.renderingBackend << "\n"
        << "Graphics Card: " << info.graphicsCard << "\n"
        << "OpenGL Version: " << info.openGLVersion << "\n"
        << "OpenGL Vendor: " << info.openGLVendor << "\n"
        << "OpenGL Renderer: " << info.openGLRenderer << "\n";
    
    if (!info.surfaceFormat.empty()) {
        oss << "Surface Format: " << info.surfaceFormat << "\n";
    }
    
#ifdef __linux__
    oss << "EGL Support: " << (info.isEglSupported ? "Yes" : "No") << "\n";
#endif
    
    if (!info.supportedExtensions.empty() && info.supportedExtensions.size() < 50) {
        oss << "OpenGL Extensions: ";
        for (size_t i = 0; i < info.supportedExtensions.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << info.supportedExtensions[i];
        }
        oss << "\n";
    } else if (!info.supportedExtensions.empty()) {
        oss << "OpenGL Extensions: " << info.supportedExtensions.size() << " extensions available\n";
    }
    
    return oss.str();
}

void HardwareAccelerationService::loadHardwareInfoAsync(InfoCallback callback) {
    std::thread([this, callback]() {
        auto info = getHardwareAccelerationInfo();
        callback(info);
    }).detach();
}

std::string HardwareAccelerationService::getPlatformInfoInternal() const {
#ifdef _WIN32
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown";
#endif
}

std::string HardwareAccelerationService::getGraphicsCardInfo() const {
#ifdef __linux__
    return getLinuxGraphicsInfo();
#elif defined(_WIN32)
    return getWindowsGraphicsInfo();
#else
    return "Platform not supported for graphics detection";
#endif
}

std::string HardwareAccelerationService::getLinuxGraphicsInfo() const {
    // First, try alternative methods that don't require external commands
    std::string graphicsInfo = getLinuxGraphicsInfoFromSysfs();
    if (!graphicsInfo.empty()) {
        return graphicsInfo;
    }

    // Try lspci only if available
    if (isCommandAvailable("lspci")) {
        std::string command = "lspci -nn 2>/dev/null | grep -i vga";
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

    // Fallback methods
    if (std::filesystem::exists("/proc/driver/nvidia/version")) {
        return "NVIDIA GPU (driver detected)";
    }
    
    // Check for AMD GPU
    if (std::filesystem::exists("/sys/class/drm")) {
        std::filesystem::path drmPath("/sys/class/drm");
        for (const auto& entry : std::filesystem::directory_iterator(drmPath)) {
            if (entry.is_directory() && entry.path().filename().string().find("card") == 0) {
                return "GPU detected via DRM";
            }
        }
    }

    return "Graphics hardware detection limited in container environment";
}

std::string HardwareAccelerationService::getLinuxGraphicsInfoFromSysfs() const {
    std::filesystem::path drmPath("/sys/class/drm");
    if (!std::filesystem::exists(drmPath)) {
        return "";
    }

    std::vector<std::string> cards;
    for (const auto& entry : std::filesystem::directory_iterator(drmPath)) {
        if (entry.is_directory()) {
            std::string cardName = entry.path().filename().string();
            // Filter out card*-* entries, keep only card[0-9]+
            if (std::regex_match(cardName, std::regex("card\\d+"))) {
                cards.push_back(cardName);
            }
        }
    }

    if (cards.empty()) {
        return "";
    }

    std::vector<std::string> gpuInfo;
    
    for (const std::string& card : cards) {
        std::filesystem::path devicePath = drmPath / card / "device";
        
        // Try to read vendor and device info
        std::filesystem::path vendorFile = devicePath / "vendor";
        std::filesystem::path deviceFile = devicePath / "device";

        if (std::filesystem::exists(vendorFile) && std::filesystem::exists(deviceFile)) {
            std::ifstream vFile(vendorFile);
            std::ifstream dFile(deviceFile);
            
            if (vFile.is_open() && dFile.is_open()) {
                std::string vendor, device;
                std::getline(vFile, vendor);
                std::getline(dFile, device);
                
                // Remove whitespace
                vendor.erase(vendor.find_last_not_of(" \n\r\t") + 1);
                device.erase(device.find_last_not_of(" \n\r\t") + 1);
                
                std::string vendorName = getVendorName(vendor);
                gpuInfo.push_back(vendorName + " GPU (" + card + ") - Vendor: " + vendor + ", Device: " + device);
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
    
    std::string result = "Graphics devices detected: ";
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) result += ", ";
        result += cards[i];
    }
    return result;
}

std::string HardwareAccelerationService::getWindowsGraphicsInfo() const {
    return "Windows Graphics Device";
}

std::string HardwareAccelerationService::getRenderingBackend() const {
#ifdef __linux__
    // Check if we're running in a framebuffer environment
    const char* displayEnv = std::getenv("DISPLAY");
    const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
    
    if ((!displayEnv || strlen(displayEnv) == 0) && (!waylandDisplay || strlen(waylandDisplay) == 0)) {
        // Likely framebuffer mode
        if (std::filesystem::exists("/sys/class/graphics")) {
            std::filesystem::path graphicsPath("/sys/class/graphics");
            for (const auto& entry : std::filesystem::directory_iterator(graphicsPath)) {
                if (entry.is_directory() && entry.path().filename().string().find("fb") == 0) {
                    return "Linux Framebuffer";
                }
            }
        }
        return "Linux (No Display Server)";
    }
    else if (waylandDisplay && strlen(waylandDisplay) > 0) {
        return "Wayland";
    }
    else if (displayEnv && strlen(displayEnv) > 0) {
        return "X11";
    }
    
    return "Linux (Display Server Unknown)";
#elif defined(_WIN32)
    return "Win32";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown";
#endif
}

bool HardwareAccelerationService::checkEglSupport() const {
#ifdef __linux__
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
        "/opt/vc/lib/libEGL.so", // Raspberry Pi
        "/usr/local/lib/libEGL.so"
    };

    for (const std::string& path : eglPaths) {
        if (std::filesystem::exists(path)) {
            std::cout << "Found EGL library at: " << path << std::endl;
            return true;
        }
    }

    // Check LD_LIBRARY_PATH for EGL libraries
    const char* ldLibraryPath = std::getenv("LD_LIBRARY_PATH");
    if (ldLibraryPath) {
        std::string pathsStr(ldLibraryPath);
        std::istringstream iss(pathsStr);
        std::string path;
        
        while (std::getline(iss, path, ':')) {
            if (std::filesystem::exists(path)) {
                for (const auto& entry : std::filesystem::directory_iterator(path)) {
                    if (entry.is_regular_file() && 
                        entry.path().filename().string().find("libEGL.so") == 0) {
                        std::cout << "Found EGL library in LD_LIBRARY_PATH: " << entry.path() << std::endl;
                        return true;
                    }
                }
            }
        }
    }

    // Try to run eglinfo if available (but don't fail if missing)
    if (isCommandAvailable("eglinfo")) {
        int result = std::system("eglinfo >/dev/null 2>&1");
        if (result == 0) {
            std::cout << "eglinfo command executed successfully" << std::endl;
            return true;
        }
    }
#endif

    return false;
}

bool HardwareAccelerationService::isCommandAvailable(const std::string& command) const {
    std::string whichCmd = "which " + command + " >/dev/null 2>&1";
    int result = std::system(whichCmd.c_str());
    if (result == 0) {
        return true;
    }
    
    // Alternative approach
    std::string versionCmd = command + " --version >/dev/null 2>&1";
    result = std::system(versionCmd.c_str());
    return (result == 0);
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
    HardwareAccelerationInfo info;
    
    // For Slint applications, we'll provide basic OpenGL detection
    // This is a simplified version that doesn't require creating OpenGL contexts
    info.openGLVersion = "OpenGL detection requires runtime context";
    info.openGLVendor = "Unknown";
    info.openGLRenderer = "Unknown";
    info.surfaceFormat = "Not available without GL context";
    
    // On Linux, we can try to get some basic GL info from system
#ifdef __linux__
    // Try to get GL info from glxinfo if available
    if (isCommandAvailable("glxinfo")) {
        FILE* pipe = popen("glxinfo 2>/dev/null | head -20", "r");
        if (pipe) {
            char buffer[256];
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
                if (result.find("OpenGL vendor string:") != std::string::npos) {
                    std::string line = buffer;
                    size_t pos = line.find(":");
                    if (pos != std::string::npos) {
                        info.openGLVendor = line.substr(pos + 1);
                        info.openGLVendor.erase(0, info.openGLVendor.find_first_not_of(" \t"));
                        info.openGLVendor.erase(info.openGLVendor.find_last_not_of(" \t\n\r") + 1);
                    }
                }
                if (result.find("OpenGL renderer string:") != std::string::npos) {
                    std::string line = buffer;
                    size_t pos = line.find(":");
                    if (pos != std::string::npos) {
                        info.openGLRenderer = line.substr(pos + 1);
                        info.openGLRenderer.erase(0, info.openGLRenderer.find_first_not_of(" \t"));
                        info.openGLRenderer.erase(info.openGLRenderer.find_last_not_of(" \t\n\r") + 1);
                    }
                }
                if (result.find("OpenGL version string:") != std::string::npos) {
                    std::string line = buffer;
                    size_t pos = line.find(":");
                    if (pos != std::string::npos) {
                        info.openGLVersion = line.substr(pos + 1);
                        info.openGLVersion.erase(0, info.openGLVersion.find_first_not_of(" \t"));
                        info.openGLVersion.erase(info.openGLVersion.find_last_not_of(" \t\n\r") + 1);
                    }
                }
            }
            pclose(pipe);
        }
    }
    
    // If glxinfo didn't work, try alternative methods
    if (info.openGLRenderer == "Unknown") {
        // Check if we're in a software rendering environment
        if (std::filesystem::exists("/usr/lib/x86_64-linux-gnu/dri/swrast_dri.so") ||
            std::filesystem::exists("/usr/lib/dri/swrast_dri.so")) {
            info.openGLRenderer = "Software Rasterizer (Mesa)";
        }
    }
#endif
    
    return info;
}