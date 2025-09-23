use std::collections::HashMap;
use std::fs;
use std::path::Path;
use std::process::Command;
use std::sync::{Arc, Mutex};
use std::thread;

#[derive(Debug, Clone)]
pub struct HardwareAccelerationInfo {
    pub is_hardware_accelerated: bool,
    pub rendering_backend: String,
    pub opengl_version: String,
    pub opengl_vendor: String,
    pub opengl_renderer: String,
    pub graphics_card: String,
    pub supported_extensions: Vec<String>,
    pub is_egl_supported: bool,
    pub is_software_rendering: bool,
    pub platform: String,
    pub surface_format: String,
}

impl Default for HardwareAccelerationInfo {
    fn default() -> Self {
        Self {
            is_hardware_accelerated: false,
            rendering_backend: "Unknown".to_string(),
            opengl_version: "Unknown".to_string(),
            opengl_vendor: "Unknown".to_string(),
            opengl_renderer: "Unknown".to_string(),
            graphics_card: "Unknown".to_string(),
            supported_extensions: Vec::new(),
            is_egl_supported: false,
            is_software_rendering: false,
            platform: "Unknown".to_string(),
            surface_format: "Unknown".to_string(),
        }
    }
}

pub struct HardwareAccelerationService {
    cached_info: Arc<Mutex<Option<HardwareAccelerationInfo>>>,
}

impl HardwareAccelerationService {
    pub fn new() -> Self {
        Self {
            cached_info: Arc::new(Mutex::new(None)),
        }
    }

    pub fn get_hardware_acceleration_info(&self) -> HardwareAccelerationInfo {
        if let Ok(cache) = self.cached_info.lock() {
            if let Some(ref info) = *cache {
                return info.clone();
            }
        }

        let mut info = HardwareAccelerationInfo::default();

        // Detect platform
        info.platform = self.get_platform_info();

        // Get graphics card information
        info.graphics_card = self.get_graphics_card_info();

        // Get OpenGL context information
        let gl_info = self.get_opengl_info();
        info.opengl_version = gl_info.opengl_version;
        info.opengl_vendor = gl_info.opengl_vendor;
        info.opengl_renderer = gl_info.opengl_renderer;
        info.supported_extensions = gl_info.supported_extensions;
        info.surface_format = gl_info.surface_format;

        // Determine if hardware accelerated
        let renderer_lower = info.opengl_renderer.to_lowercase();
        info.is_hardware_accelerated = !renderer_lower.contains("software")
            && !renderer_lower.contains("llvmpipe")
            && !renderer_lower.contains("mesa software");

        // Detect rendering backend
        info.rendering_backend = self.get_rendering_backend();

        // Check if software rendering is being used
        info.is_software_rendering = info.rendering_backend.contains("Software")
            || renderer_lower.contains("software")
            || renderer_lower.contains("llvmpipe")
            || renderer_lower.contains("mesa software");

        // Check EGL support on Linux
        if cfg!(target_os = "linux") {
            info.is_egl_supported = self.check_egl_support();
        }

        // Cache the result
        if let Ok(mut cache) = self.cached_info.lock() {
            *cache = Some(info.clone());
        }

        info
    }

    pub fn get_hardware_info_text(&self, info: &HardwareAccelerationInfo) -> String {
        format!(
            "Platform: {}\nHardware Accelerated: {}\nRendering Backend: {}\nGraphics Card: {}\nOpenGL Renderer: {}",
            info.platform,
            if info.is_hardware_accelerated { "✅ Yes" } else { "❌ No" },
            info.rendering_backend,
            info.graphics_card,
            info.opengl_renderer
        )
    }

    pub fn get_detailed_report(&self, info: &HardwareAccelerationInfo) -> String {
        let mut report = String::new();
        report.push_str("=== Hardware Acceleration Report ===\n");
        report.push_str(&format!("Platform: {}\n", info.platform));
        report.push_str(&format!(
            "Hardware Accelerated: {}\n",
            if info.is_hardware_accelerated { "Yes" } else { "No" }
        ));
        report.push_str(&format!(
            "Software Rendering: {}\n",
            if info.is_software_rendering { "Yes" } else { "No" }
        ));
        report.push_str(&format!("Rendering Backend: {}\n", info.rendering_backend));
        report.push_str(&format!("Graphics Card: {}\n", info.graphics_card));
        report.push_str(&format!("OpenGL Version: {}\n", info.opengl_version));
        report.push_str(&format!("OpenGL Vendor: {}\n", info.opengl_vendor));
        report.push_str(&format!("OpenGL Renderer: {}\n", info.opengl_renderer));

        if !info.surface_format.is_empty() && info.surface_format != "Unknown" {
            report.push_str(&format!("Surface Format: {}\n", info.surface_format));
        }

        if cfg!(target_os = "linux") {
            report.push_str(&format!(
                "EGL Support: {}\n",
                if info.is_egl_supported { "Yes" } else { "No" }
            ));
        }

        if !info.supported_extensions.is_empty() && info.supported_extensions.len() < 50 {
            report.push_str(&format!(
                "OpenGL Extensions: {}\n",
                info.supported_extensions.join(", ")
            ));
        } else if !info.supported_extensions.is_empty() {
            report.push_str(&format!(
                "OpenGL Extensions: {} extensions available\n",
                info.supported_extensions.len()
            ));
        }

        report
    }

    pub fn load_hardware_info_async<F>(&self, callback: F)
    where
        F: FnOnce(HardwareAccelerationInfo) + Send + 'static,
    {
        let service = self.clone();
        thread::spawn(move || {
            let info = service.get_hardware_acceleration_info();
            callback(info);
        });
    }

    fn get_platform_info(&self) -> String {
        if cfg!(target_os = "windows") {
            "Windows".to_string()
        } else if cfg!(target_os = "linux") {
            "Linux".to_string()
        } else if cfg!(target_os = "macos") {
            "macOS".to_string()
        } else {
            std::env::consts::OS.to_string()
        }
    }

    fn get_graphics_card_info(&self) -> String {
        if cfg!(target_os = "linux") {
            self.get_linux_graphics_info()
        } else if cfg!(target_os = "windows") {
            self.get_windows_graphics_info()
        } else {
            "Platform not supported for graphics detection".to_string()
        }
    }

    fn get_linux_graphics_info(&self) -> String {
        // First, try sysfs-based detection
        if let Ok(graphics_info) = self.get_linux_graphics_info_from_sysfs() {
            if !graphics_info.is_empty() {
                return graphics_info;
            }
        }

        // Try lspci only if available
        if self.is_command_available("lspci") {
            if let Ok(output) = Command::new("lspci")
                .args(["-nn"])
                .output()
            {
                let stdout = String::from_utf8_lossy(&output.stdout);
                for line in stdout.lines() {
                    let line_lower = line.to_lowercase();
                    if line_lower.contains("vga") || line_lower.contains("3d") {
                        return line.trim().to_string();
                    }
                }
            }
        }

        // Fallback methods
        if Path::new("/proc/driver/nvidia/version").exists() {
            return "NVIDIA GPU (driver detected)".to_string();
        }

        // Check for AMD GPU via DRM
        if Path::new("/sys/class/drm").exists() {
            if let Ok(entries) = fs::read_dir("/sys/class/drm") {
                let cards: Vec<_> = entries
                    .filter_map(|entry| entry.ok())
                    .filter(|entry| {
                        if let Some(name) = entry.file_name().to_str() {
                            name.starts_with("card") && !name.contains('-')
                        } else {
                            false
                        }
                    })
                    .collect();

                if !cards.is_empty() {
                    return "GPU detected via DRM".to_string();
                }
            }
        }

        "Graphics hardware detection limited in container environment".to_string()
    }

    fn get_linux_graphics_info_from_sysfs(&self) -> Result<String, Box<dyn std::error::Error>> {
        let drm_path = Path::new("/sys/class/drm");
        if !drm_path.exists() {
            return Ok(String::new());
        }

        let mut cards = Vec::new();
        for entry in fs::read_dir(drm_path)? {
            let entry = entry?;
            if let Some(name) = entry.file_name().to_str() {
                if name.starts_with("card") && !name.contains('-') {
                    cards.push(name.to_string());
                }
            }
        }

        if cards.is_empty() {
            return Ok(String::new());
        }

        let mut gpu_info = Vec::new();

        for card in &cards {
            let device_path = drm_path.join(card).join("device");
            let vendor_file = device_path.join("vendor");
            let device_file = device_path.join("device");

            if vendor_file.exists() && device_file.exists() {
                if let (Ok(vendor), Ok(device)) = (
                    fs::read_to_string(&vendor_file),
                    fs::read_to_string(&device_file),
                ) {
                    let vendor = vendor.trim();
                    let device = device.trim();
                    let vendor_name = self.get_vendor_name(vendor);

                    gpu_info.push(format!(
                        "{} GPU ({}) - Vendor: {}, Device: {}",
                        vendor_name, card, vendor, device
                    ));
                }
            }
        }

        if !gpu_info.is_empty() {
            Ok(gpu_info.join("; "))
        } else {
            Ok(format!("Graphics devices detected: {}", cards.join(", ")))
        }
    }

    fn get_windows_graphics_info(&self) -> String {
        "Windows Graphics Device".to_string()
    }

    fn get_rendering_backend(&self) -> String {
        if cfg!(target_os = "linux") {
            let display_env = std::env::var("DISPLAY").unwrap_or_default();
            let wayland_display = std::env::var("WAYLAND_DISPLAY").unwrap_or_default();

            if display_env.is_empty() && wayland_display.is_empty() {
                // Likely framebuffer mode
                if Path::new("/sys/class/graphics").exists() {
                    if let Ok(entries) = fs::read_dir("/sys/class/graphics") {
                        let has_fb = entries
                            .filter_map(|entry| entry.ok())
                            .any(|entry| {
                                if let Some(name) = entry.file_name().to_str() {
                                    name.starts_with("fb")
                                } else {
                                    false
                                }
                            });

                        if has_fb {
                            return "Linux Framebuffer".to_string();
                        }
                    }
                }
                "Linux (No Display Server)".to_string()
            } else if !wayland_display.is_empty() {
                "Wayland".to_string()
            } else if !display_env.is_empty() {
                "X11".to_string()
            } else {
                "Linux (Display Server Unknown)".to_string()
            }
        } else if cfg!(target_os = "windows") {
            "Win32".to_string()
        } else if cfg!(target_os = "macos") {
            "macOS".to_string()
        } else {
            "Unknown".to_string()
        }
    }

    fn check_egl_support(&self) -> bool {
        let egl_paths = [
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
            "/usr/local/lib/libEGL.so",
        ];

        for path in &egl_paths {
            if Path::new(path).exists() {
                println!("Found EGL library at: {}", path);
                return true;
            }
        }

        // Check LD_LIBRARY_PATH for EGL libraries
        if let Ok(ld_library_path) = std::env::var("LD_LIBRARY_PATH") {
            for path in ld_library_path.split(':') {
                let path = Path::new(path);
                if path.exists() {
                    if let Ok(entries) = fs::read_dir(path) {
                        for entry in entries.filter_map(|e| e.ok()) {
                            if let Some(name) = entry.file_name().to_str() {
                                if name.starts_with("libEGL.so") {
                                    println!("Found EGL library in LD_LIBRARY_PATH: {:?}", entry.path());
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Try to run eglinfo if available
        if self.is_command_available("eglinfo") {
            if let Ok(output) = Command::new("eglinfo").output() {
                if output.status.success() {
                    println!("eglinfo command executed successfully");
                    return true;
                }
            }
        }

        false
    }

    fn is_command_available(&self, command: &str) -> bool {
        Command::new("which")
            .arg(command)
            .output()
            .map(|output| output.status.success())
            .unwrap_or(false)
    }

    fn get_vendor_name(&self, vendor_id: &str) -> String {
        let vendor_id_lower = vendor_id.to_lowercase();
        match vendor_id_lower.as_str() {
            "0x1002" => "AMD".to_string(),
            "0x10de" => "NVIDIA".to_string(),
            "0x8086" => "Intel".to_string(),
            "0x1234" => "QEMU".to_string(),
            "0x15ad" => "VMware".to_string(),
            _ => vendor_id.to_string(),
        }
    }

    fn get_opengl_info(&self) -> HardwareAccelerationInfo {
        let mut info = HardwareAccelerationInfo::default();

        // For Rust/Slint applications, we'll use system commands to get OpenGL info
        info.opengl_version = "OpenGL detection requires runtime context".to_string();
        info.opengl_vendor = "Unknown".to_string();
        info.opengl_renderer = "Unknown".to_string();
        info.surface_format = "Not available without GL context".to_string();

        // On Linux, try to get GL info from glxinfo if available
        if cfg!(target_os = "linux") && self.is_command_available("glxinfo") {
            if let Ok(output) = Command::new("glxinfo").output() {
                let stdout = String::from_utf8_lossy(&output.stdout);
                let lines: Vec<&str> = stdout.lines().take(20).collect();

                for line in lines {
                    if line.contains("OpenGL vendor string:") {
                        if let Some(vendor) = line.split(':').nth(1) {
                            info.opengl_vendor = vendor.trim().to_string();
                        }
                    } else if line.contains("OpenGL renderer string:") {
                        if let Some(renderer) = line.split(':').nth(1) {
                            info.opengl_renderer = renderer.trim().to_string();
                        }
                    } else if line.contains("OpenGL version string:") {
                        if let Some(version) = line.split(':').nth(1) {
                            info.opengl_version = version.trim().to_string();
                        }
                    }
                }
            }
        }

        // If glxinfo didn't work, check for software rendering indicators
        if info.opengl_renderer == "Unknown" {
            let software_paths = [
                "/usr/lib/x86_64-linux-gnu/dri/swrast_dri.so",
                "/usr/lib/dri/swrast_dri.so",
            ];

            for path in &software_paths {
                if Path::new(path).exists() {
                    info.opengl_renderer = "Software Rasterizer (Mesa)".to_string();
                    break;
                }
            }
        }

        info
    }
}

impl Clone for HardwareAccelerationService {
    fn clone(&self) -> Self {
        Self {
            cached_info: Arc::clone(&self.cached_info),
        }
    }
}