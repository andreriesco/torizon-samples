#include "appwindow.h"
#include "HardwareAccelerationService.h"

#include <iostream>
#include <thread>
#include <memory>

int main(int argc, char **argv)
{
    auto ui = AppWindow::create();
    auto hardwareService = std::make_shared<HardwareAccelerationService>();

    // Function to update UI with hardware info
    auto updateHardwareInfo = [ui](const HardwareAccelerationInfo& info, 
                                   std::shared_ptr<HardwareAccelerationService> service) {
        // Update the structured hardware info
        HardwareInfo slintInfo;
        slintInfo.platform = slint::SharedString(info.platform);
        slintInfo.graphics_card = slint::SharedString(info.graphicsCard);
        slintInfo.opengl_version = slint::SharedString(info.openGLVersion);
        slintInfo.opengl_vendor = slint::SharedString(info.openGLVendor);
        slintInfo.opengl_renderer = slint::SharedString(info.openGLRenderer);
        slintInfo.rendering_backend = slint::SharedString(info.renderingBackend);
        slintInfo.is_hardware_accelerated = info.isHardwareAccelerated;
        slintInfo.is_software_rendering = info.isSoftwareRendering;
        slintInfo.is_egl_supported = info.isEglSupported;
        slintInfo.surface_format = slint::SharedString(info.surfaceFormat);
        
        ui->set_hardware_info(slintInfo);
        
        // Update text representations
        std::string hardwareInfoText = service->getHardwareInfoText(info);
        std::string detailedReport = service->getDetailedReport(info);
        
        ui->set_hardware_info_text(slint::SharedString(hardwareInfoText));
        ui->set_detailed_report(slint::SharedString(detailedReport));
        ui->set_is_loading(false);
    };

    // Set up refresh callback
    ui->on_refresh_hardware_info([ui, hardwareService, updateHardwareInfo](){
        ui->set_is_loading(true);
        
        // Load hardware info synchronously for simplicity
        // In a production app, you might want to use proper threading with Slint's timer
        auto info = hardwareService->getHardwareAccelerationInfo();
        updateHardwareInfo(info, hardwareService);
    });

    // Set up detailed report toggle
    ui->on_toggle_detailed_report([ui](){
        ui->set_show_detailed_report(!ui->get_show_detailed_report());
    });

    // Load initial hardware information
    std::cout << "\n=== C++ Slint Hardware Acceleration Detailed Log ===" << std::endl;
    std::cout << "Project: cppSlintHardAccTest" << std::endl;
    std::cout << "Framework: Slint + C++" << std::endl;
    
    // Initial load
    auto initialInfo = hardwareService->getHardwareAccelerationInfo();
    
    std::cout << "Backend: " << initialInfo.renderingBackend << std::endl;
    std::cout << "Hardware Accelerated: " << (initialInfo.isHardwareAccelerated ? "Yes" : "No") << std::endl;
    std::cout << "\n" << hardwareService->getDetailedReport(initialInfo) << std::endl;
    std::cout << "=== End of Detailed Log ===" << std::endl << std::endl;
    
    updateHardwareInfo(initialInfo, hardwareService);

    ui->run();
    return 0;
}
