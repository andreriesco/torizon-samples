#ifndef HARDWARE_ACCELERATION_SERVICE_H
#define HARDWARE_ACCELERATION_SERVICE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __has_include
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
#else
#define HAS_OPENGL 0
#define HAS_EGL 0
#endif

#define MAX_STRING_LENGTH 256
#define MAX_EXTENSIONS 32

typedef struct {
    bool is_hardware_accelerated;
    char rendering_backend[MAX_STRING_LENGTH];
    char opengl_version[MAX_STRING_LENGTH];
    char opengl_vendor[MAX_STRING_LENGTH];
    char opengl_renderer[MAX_STRING_LENGTH];
    char graphics_card[MAX_STRING_LENGTH];
    char supported_extensions[MAX_EXTENSIONS][MAX_STRING_LENGTH];
    int extension_count;
    bool is_egl_supported;
    bool is_software_rendering;
    char platform[MAX_STRING_LENGTH];
    char surface_format[MAX_STRING_LENGTH];
} hardware_acceleration_info_t;

typedef struct {
    hardware_acceleration_info_t info;
    char hardware_info_text[2048];
    char detailed_report_text[4096];
    bool is_loading;
} hardware_acceleration_service_t;

// Function prototypes
int hardware_acceleration_service_init(hardware_acceleration_service_t *service);
void hardware_acceleration_service_destroy(hardware_acceleration_service_t *service);

bool hardware_acceleration_service_is_accelerated(const hardware_acceleration_service_t *service);
const char* hardware_acceleration_service_get_info_text(const hardware_acceleration_service_t *service);
const char* hardware_acceleration_service_get_detailed_report(const hardware_acceleration_service_t *service);
bool hardware_acceleration_service_is_loading(const hardware_acceleration_service_t *service);

void hardware_acceleration_service_refresh(hardware_acceleration_service_t *service);
const char* hardware_acceleration_service_get_platform_info(const hardware_acceleration_service_t *service);

// Internal functions
static void load_hardware_info(hardware_acceleration_service_t *service);
static void get_platform_info_internal(char *platform_info, size_t size);
static void get_graphics_card_info(char *graphics_card, size_t size);
static void get_linux_graphics_info(char *graphics_info, size_t size);
static void get_linux_graphics_info_from_sysfs(char *graphics_info, size_t size);
static void get_rendering_backend(char *backend, size_t size);
static bool check_egl_support(void);
static bool is_command_available(const char *command);
static void get_vendor_name(const char *vendor_id, char *vendor_name, size_t size);
static void get_opengl_info(hardware_acceleration_info_t *info);
static void format_detailed_report(const hardware_acceleration_info_t *info, char *report, size_t size);
static void format_hardware_info_text(const hardware_acceleration_info_t *info, char *text, size_t size);

#endif // HARDWARE_ACCELERATION_SERVICE_H