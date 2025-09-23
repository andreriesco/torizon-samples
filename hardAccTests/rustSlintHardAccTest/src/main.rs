mod hardware_service;

use slint::Model;
use std::rc::Rc;
use hardware_service::{HardwareAccelerationService, HardwareAccelerationInfo};

slint::include_modules!();

// Convert our Rust struct to Slint's HardwareInfo
fn convert_to_slint_hardware_info(info: &HardwareAccelerationInfo) -> HardwareInfo {
    HardwareInfo {
        is_hardware_accelerated: info.is_hardware_accelerated,
        rendering_backend: info.rendering_backend.clone().into(),
        opengl_version: info.opengl_version.clone().into(),
        opengl_vendor: info.opengl_vendor.clone().into(),
        opengl_renderer: info.opengl_renderer.clone().into(),
        graphics_card: info.graphics_card.clone().into(),
        is_egl_supported: info.is_egl_supported,
        is_software_rendering: info.is_software_rendering,
        platform: info.platform.clone().into(),
        surface_format: info.surface_format.clone().into(),
        detailed_report: "".into(), // Will be set separately
    }
}

fn main() -> Result<(), slint::PlatformError> {
    let ui = AppWindow::new()?;
    let hardware_service = HardwareAccelerationService::new();
    
    // Print detailed hardware acceleration log
    println!("\n=== Rust Slint Hardware Acceleration Detailed Log ===");
    println!("Project: rustSlintHardAccTest");
    println!("Framework: Slint + Rust");
    
    // Get initial hardware info for logging
    let log_info = hardware_service.get_hardware_acceleration_info();
    println!("Backend: {}", log_info.rendering_backend);
    println!("Hardware Accelerated: {}", if log_info.is_hardware_accelerated { "Yes" } else { "No" });
    println!("\n{}", hardware_service.get_detailed_report(&log_info));
    println!("=== End of Detailed Log ===\n");
    
    // Initial load for UI
    let ui_weak = ui.as_weak();
    let service_clone = hardware_service.clone();
    
    std::thread::spawn(move || {
        let info = service_clone.get_hardware_acceleration_info();
        let detailed_report = service_clone.get_detailed_report(&info);
        
        slint::invoke_from_event_loop(move || {
            if let Some(ui) = ui_weak.upgrade() {
                let mut slint_info = convert_to_slint_hardware_info(&info);
                slint_info.detailed_report = detailed_report.into();
                
                ui.set_hardware_info(slint_info);
                ui.set_is_loading(false);
            }
        }).unwrap();
    });
    
    // Set up refresh callback
    let ui_weak_refresh = ui.as_weak();
    let service_refresh = hardware_service.clone();
    ui.on_refresh_hardware_info(move || {
        let ui_weak_refresh = ui_weak_refresh.clone();
        let service_refresh = service_refresh.clone();
        
        // Show loading state
        if let Some(ui) = ui_weak_refresh.upgrade() {
            ui.set_is_loading(true);
        }
        
        std::thread::spawn(move || {
            let info = service_refresh.get_hardware_acceleration_info();
            let detailed_report = service_refresh.get_detailed_report(&info);
            
            slint::invoke_from_event_loop(move || {
                if let Some(ui) = ui_weak_refresh.upgrade() {
                    let mut slint_info = convert_to_slint_hardware_info(&info);
                    slint_info.detailed_report = detailed_report.into();
                    
                    ui.set_hardware_info(slint_info);
                    ui.set_is_loading(false);
                }
            }).unwrap();
        });
    });
    
    // Set up toggle details callback
    let ui_weak_details = ui.as_weak();
    ui.on_toggle_details(move || {
        if let Some(ui) = ui_weak_details.upgrade() {
            ui.set_show_details(!ui.get_show_details());
        }
    });

    ui.run()
}
