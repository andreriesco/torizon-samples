#include "../includes/hardware_acceleration_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <sys/stat.h>

// Helper function to safely copy strings
static void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (dest && src && dest_size > 0) {
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

// Helper function to safely concatenate strings
static void safe_strcat(char *dest, const char *src, size_t dest_size) {
    if (dest && src && dest_size > 0) {
        size_t current_len = strlen(dest);
        if (current_len < dest_size - 1) {
            strncat(dest, src, dest_size - current_len - 1);
        }
    }
}

int hardware_acceleration_service_init(hardware_acceleration_service_t *service) {
    if (!service) {
        return -1;
    }
    
    memset(service, 0, sizeof(hardware_acceleration_service_t));
    
    // Initialize with default values
    safe_strcpy(service->info.rendering_backend, "Unknown", sizeof(service->info.rendering_backend));
    safe_strcpy(service->info.opengl_version, "Unknown", sizeof(service->info.opengl_version));
    safe_strcpy(service->info.opengl_vendor, "Unknown", sizeof(service->info.opengl_vendor));
    safe_strcpy(service->info.opengl_renderer, "Unknown", sizeof(service->info.opengl_renderer));
    safe_strcpy(service->info.graphics_card, "Unknown", sizeof(service->info.graphics_card));
    safe_strcpy(service->info.platform, "Unknown", sizeof(service->info.platform));
    safe_strcpy(service->info.surface_format, "Unknown", sizeof(service->info.surface_format));
    
    // Load initial hardware info
    load_hardware_info(service);
    
    return 0;
}

void hardware_acceleration_service_destroy(hardware_acceleration_service_t *service) {
    if (service) {
        memset(service, 0, sizeof(hardware_acceleration_service_t));
    }
}

bool hardware_acceleration_service_is_accelerated(const hardware_acceleration_service_t *service) {
    return service ? service->info.is_hardware_accelerated : false;
}

const char* hardware_acceleration_service_get_info_text(const hardware_acceleration_service_t *service) {
    return service ? service->hardware_info_text : "";
}

const char* hardware_acceleration_service_get_detailed_report(const hardware_acceleration_service_t *service) {
    return service ? service->detailed_report_text : "";
}

bool hardware_acceleration_service_is_loading(const hardware_acceleration_service_t *service) {
    return service ? service->is_loading : false;
}

void hardware_acceleration_service_refresh(hardware_acceleration_service_t *service) {
    if (service) {
        load_hardware_info(service);
    }
}

const char* hardware_acceleration_service_get_platform_info(const hardware_acceleration_service_t *service) {
    return service ? service->info.platform : "";
}

static void load_hardware_info(hardware_acceleration_service_t *service) {
    if (!service) {
        return;
    }
    
    service->is_loading = true;
    
    // Get platform information
    get_platform_info_internal(service->info.platform, sizeof(service->info.platform));
    
    // Get graphics card information
    get_graphics_card_info(service->info.graphics_card, sizeof(service->info.graphics_card));
    
    // Get OpenGL information
    get_opengl_info(&service->info);
    
    // Determine if hardware accelerated
    char renderer_lower[MAX_STRING_LENGTH];
    safe_strcpy(renderer_lower, service->info.opengl_renderer, sizeof(renderer_lower));
    
    // Convert to lowercase for comparison
    for (int i = 0; renderer_lower[i]; i++) {
        if (renderer_lower[i] >= 'A' && renderer_lower[i] <= 'Z') {
            renderer_lower[i] = renderer_lower[i] + ('a' - 'A');
        }
    }
    
    service->info.is_hardware_accelerated = 
        (strstr(renderer_lower, "software") == NULL) &&
        (strstr(renderer_lower, "llvmpipe") == NULL) &&
        (strstr(renderer_lower, "mesa software") == NULL);
    
    service->info.is_software_rendering = !service->info.is_hardware_accelerated;
    
    // Get rendering backend
    get_rendering_backend(service->info.rendering_backend, sizeof(service->info.rendering_backend));
    
    // Check EGL support
    service->info.is_egl_supported = check_egl_support();
    
    // Format output texts
    format_hardware_info_text(&service->info, service->hardware_info_text, sizeof(service->hardware_info_text));
    format_detailed_report(&service->info, service->detailed_report_text, sizeof(service->detailed_report_text));
    
    service->is_loading = false;
}

static void get_platform_info_internal(char *platform_info, size_t size) {
    struct utsname uname_data;
    if (uname(&uname_data) == 0) {
        snprintf(platform_info, size, "%s %s %s", 
                uname_data.sysname, uname_data.release, uname_data.machine);
    } else {
        safe_strcpy(platform_info, "Linux (unknown version)", size);
    }
}

static void get_graphics_card_info(char *graphics_card, size_t size) {
    // First try sysfs approach
    get_linux_graphics_info_from_sysfs(graphics_card, size);
    if (strlen(graphics_card) > 0) {
        return;
    }
    
    // Fallback to lspci if available
    get_linux_graphics_info(graphics_card, size);
}

static void get_linux_graphics_info(char *graphics_info, size_t size) {
    FILE *pipe;
    char buffer[256];
    
    // Try lspci if available
    if (is_command_available("lspci")) {
        pipe = popen("lspci -nn 2>/dev/null | grep -i 'vga\\|3d\\|display'", "r");
        if (pipe) {
            if (fgets(buffer, sizeof(buffer), pipe)) {
                // Remove trailing newline
                buffer[strcspn(buffer, "\n")] = '\0';
                safe_strcpy(graphics_info, buffer, size);
                pclose(pipe);
                return;
            }
            pclose(pipe);
        }
    }
    
    // Check for NVIDIA driver
    if (access("/proc/driver/nvidia/version", F_OK) == 0) {
        safe_strcpy(graphics_info, "NVIDIA GPU (driver detected)", size);
        return;
    }
    
    // Check for AMD GPU via DRM
    DIR *drm_dir = opendir("/sys/class/drm");
    if (drm_dir) {
        struct dirent *entry;
        while ((entry = readdir(drm_dir)) != NULL) {
            if (strncmp(entry->d_name, "card", 4) == 0) {
                safe_strcpy(graphics_info, "GPU detected via DRM", size);
                closedir(drm_dir);
                return;
            }
        }
        closedir(drm_dir);
    }
    
    safe_strcpy(graphics_info, "Graphics hardware detection limited in container environment", size);
}

static void get_linux_graphics_info_from_sysfs(char *graphics_info, size_t size) {
    DIR *drm_dir;
    struct dirent *entry;
    char card_path[512];
    char vendor_path[512];
    char device_path[512];
    FILE *vendor_file, *device_file;
    char vendor_id[32], device_id[32];
    char vendor_name[64];
    bool found_gpu = false;
    
    graphics_info[0] = '\0';
    
    drm_dir = opendir("/sys/class/drm");
    if (!drm_dir) {
        return;
    }
    
    while ((entry = readdir(drm_dir)) != NULL) {
        // Filter out card*-* entries, keep only card[0-9]+
        if (strncmp(entry->d_name, "card", 4) == 0) {
            bool is_simple_card = true;
            for (int i = 4; entry->d_name[i]; i++) {
                if ((entry->d_name[i] < '0' || entry->d_name[i] > '9') && entry->d_name[i] != '\0') {
                    is_simple_card = false;
                    break;
                }
            }
            
            if (!is_simple_card) {
                continue;
            }
            
            snprintf(card_path, sizeof(card_path), "/sys/class/drm/%s/device", entry->d_name);
            snprintf(vendor_path, sizeof(vendor_path), "%s/vendor", card_path);
            snprintf(device_path, sizeof(device_path), "%s/device", card_path);
            
            vendor_file = fopen(vendor_path, "r");
            device_file = fopen(device_path, "r");
            
            if (vendor_file && device_file) {
                if (fgets(vendor_id, sizeof(vendor_id), vendor_file) &&
                    fgets(device_id, sizeof(device_id), device_file)) {
                    
                    // Remove newlines
                    vendor_id[strcspn(vendor_id, "\n")] = '\0';
                    device_id[strcspn(device_id, "\n")] = '\0';
                    
                    get_vendor_name(vendor_id, vendor_name, sizeof(vendor_name));
                    
                    if (found_gpu) {
                        safe_strcat(graphics_info, "; ", size);
                    }
                    
                    char gpu_info[256];
                    snprintf(gpu_info, sizeof(gpu_info), "%s GPU (%s) - Vendor: %s, Device: %s",
                            vendor_name, entry->d_name, vendor_id, device_id);
                    safe_strcat(graphics_info, gpu_info, size);
                    found_gpu = true;
                }
            }
            
            if (vendor_file) fclose(vendor_file);
            if (device_file) fclose(device_file);
        }
    }
    
    closedir(drm_dir);
}

static void get_rendering_backend(char *backend, size_t size) {
    safe_strcpy(backend, "LVGL", size);
}

static bool check_egl_support(void) {
#if HAS_EGL
    // Check if EGL libraries are available in common locations
    const char *egl_paths[] = {
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
        "/usr/local/lib/libEGL.so",
        NULL
    };
    
    for (int i = 0; egl_paths[i]; i++) {
        if (access(egl_paths[i], F_OK) == 0) {
            printf("Found EGL library at: %s\n", egl_paths[i]);
            return true;
        }
    }
    
    // Try to run eglinfo if available
    if (is_command_available("eglinfo")) {
        int result = system("eglinfo >/dev/null 2>&1");
        if (result == 0) {
            printf("eglinfo command executed successfully\n");
            return true;
        }
    }
#endif
    
    return false;
}

static bool is_command_available(const char *command) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "which %s >/dev/null 2>&1", command);
    return system(cmd) == 0;
}

static void get_vendor_name(const char *vendor_id, char *vendor_name, size_t size) {
    if (strcmp(vendor_id, "0x1002") == 0) {
        safe_strcpy(vendor_name, "AMD", size);
    } else if (strcmp(vendor_id, "0x10de") == 0) {
        safe_strcpy(vendor_name, "NVIDIA", size);
    } else if (strcmp(vendor_id, "0x8086") == 0) {
        safe_strcpy(vendor_name, "Intel", size);
    } else if (strcmp(vendor_id, "0x1234") == 0) {
        safe_strcpy(vendor_name, "QEMU", size);
    } else if (strcmp(vendor_id, "0x15ad") == 0) {
        safe_strcpy(vendor_name, "VMware", size);
    } else {
        safe_strcpy(vendor_name, vendor_id, size);
    }
}

static void get_opengl_info(hardware_acceleration_info_t *info) {
    safe_strcpy(info->opengl_version, "OpenGL detection requires runtime context", sizeof(info->opengl_version));
    safe_strcpy(info->opengl_vendor, "Unknown", sizeof(info->opengl_vendor));
    safe_strcpy(info->opengl_renderer, "Unknown", sizeof(info->opengl_renderer));
    safe_strcpy(info->surface_format, "Unknown", sizeof(info->surface_format));
    
#if HAS_OPENGL
    // On Linux, we can try to get some basic GL info from glxinfo if available
    if (is_command_available("glxinfo")) {
        FILE *pipe = popen("glxinfo 2>/dev/null", "r");
        if (pipe) {
            char line[256];
            while (fgets(line, sizeof(line), pipe)) {
                if (strstr(line, "OpenGL vendor string:")) {
                    char *colon = strchr(line, ':');
                    if (colon && colon[1]) {
                        char *vendor = colon + 1;
                        // Trim leading spaces
                        while (*vendor == ' ' || *vendor == '\t') vendor++;
                        // Remove trailing newline
                        vendor[strcspn(vendor, "\n")] = '\0';
                        safe_strcpy(info->opengl_vendor, vendor, sizeof(info->opengl_vendor));
                    }
                } else if (strstr(line, "OpenGL renderer string:")) {
                    char *colon = strchr(line, ':');
                    if (colon && colon[1]) {
                        char *renderer = colon + 1;
                        // Trim leading spaces
                        while (*renderer == ' ' || *renderer == '\t') renderer++;
                        // Remove trailing newline
                        renderer[strcspn(renderer, "\n")] = '\0';
                        safe_strcpy(info->opengl_renderer, renderer, sizeof(info->opengl_renderer));
                    }
                } else if (strstr(line, "OpenGL version string:")) {
                    char *colon = strchr(line, ':');
                    if (colon && colon[1]) {
                        char *version = colon + 1;
                        // Trim leading spaces
                        while (*version == ' ' || *version == '\t') version++;
                        // Remove trailing newline
                        version[strcspn(version, "\n")] = '\0';
                        safe_strcpy(info->opengl_version, version, sizeof(info->opengl_version));
                    }
                }
            }
            pclose(pipe);
        }
    }
    
    // If glxinfo didn't work, try alternative methods
    if (strcmp(info->opengl_renderer, "Unknown") == 0) {
        // Check if we're in a software rendering environment
        if (access("/usr/lib/x86_64-linux-gnu/dri/swrast_dri.so", F_OK) == 0 ||
            access("/usr/lib/dri/swrast_dri.so", F_OK) == 0) {
            safe_strcpy(info->opengl_renderer, "Software Rasterizer (Mesa)", sizeof(info->opengl_renderer));
        }
    }
#endif
}

static void format_detailed_report(const hardware_acceleration_info_t *info, char *report, size_t size) {
    snprintf(report, size,
        "=== Hardware Acceleration Report ===\n"
        "Platform: %s\n"
        "Hardware Accelerated: %s\n"
        "Software Rendering: %s\n"
        "Rendering Backend: %s\n"
        "Graphics Card: %s\n"
        "OpenGL Version: %s\n"
        "OpenGL Vendor: %s\n"
        "OpenGL Renderer: %s\n"
        "Surface Format: %s\n"
        "EGL Support: %s\n",
        info->platform,
        info->is_hardware_accelerated ? "Yes" : "No",
        info->is_software_rendering ? "Yes" : "No",
        info->rendering_backend,
        info->graphics_card,
        info->opengl_version,
        info->opengl_vendor,
        info->opengl_renderer,
        info->surface_format,
        info->is_egl_supported ? "Yes" : "No"
    );
    
    if (info->extension_count > 0) {
        safe_strcat(report, "Supported Extensions: ", size);
        for (int i = 0; i < info->extension_count && i < MAX_EXTENSIONS; i++) {
            if (i > 0) safe_strcat(report, ", ", size);
            safe_strcat(report, info->supported_extensions[i], size);
        }
        safe_strcat(report, "\n", size);
    }
}

static void format_hardware_info_text(const hardware_acceleration_info_t *info, char *text, size_t size) {
    snprintf(text, size,
        "Platform: %s\n"
        "Hardware Accelerated: %s\n"
        "Rendering Backend: %s\n"
        "Graphics Card: %s\n"
        "OpenGL Renderer: %s",
        info->platform,
        info->is_hardware_accelerated ? "✅ Yes" : "❌ No",
        info->rendering_backend,
        info->graphics_card,
        info->opengl_renderer
    );
}