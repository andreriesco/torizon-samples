#include <gtk/gtk.h>
#include <iostream>
#include <memory>
#include <glib.h>
#include "HardwareAccelerationService.h"

class HardwareAccelerationApp {
public:
    HardwareAccelerationApp() : m_hardwareService(std::make_unique<HardwareAccelerationService>()) {}
    
    void run(int argc, char** argv) {
        GtkApplication *app;
        int status;

        app = gtk_application_new("org.gtk.hardwareacceleration", G_APPLICATION_FLAGS_NONE);
        g_signal_connect(app, "activate", G_CALLBACK(activate_callback), this);

        status = g_application_run(G_APPLICATION(app), argc, argv);
        g_object_unref(app);
    }

private:
    static void activate_callback(GtkApplication *app, gpointer user_data) {
        static_cast<HardwareAccelerationApp*>(user_data)->activate(app);
    }
    
    static void refresh_button_clicked(GtkWidget *widget, gpointer user_data) {
        static_cast<HardwareAccelerationApp*>(user_data)->refreshHardwareInfo();
    }
    
    static void toggle_detailed_report(GtkWidget *widget, gpointer user_data) {
        static_cast<HardwareAccelerationApp*>(user_data)->toggleDetailedReport();
    }

    void activate(GtkApplication *app) {
        // Print detailed hardware acceleration log
        auto info = m_hardwareService->getHardwareAccelerationInfo();
        std::cout << "\n=== C++ GTK4 Hardware Acceleration Detailed Log ===" << std::endl;
        std::cout << "Project: cmakeGTK4HardAccTest" << std::endl;
        std::cout << "Framework: GTK4 + C++" << std::endl;
        std::cout << "Backend: " << info.renderingBackend << std::endl;
        std::cout << "Hardware Accelerated: " << (info.isHardwareAccelerated ? "Yes" : "No") << std::endl;
        std::cout << "\n" << m_hardwareService->getDetailedReport(info) << std::endl;
        std::cout << "=== End of Detailed Log ===" << std::endl << std::endl;
        
        // Create main window
        m_window = gtk_application_window_new(app);
        gtk_window_set_title(GTK_WINDOW(m_window), "Hardware Acceleration Test - GTK4 with Torizon");
        gtk_window_set_default_size(GTK_WINDOW(m_window), 800, 600);

        // Load custom CSS for styling
        loadCustomCSS();

        // Create scrolled window
        GtkWidget *scrolled_window = gtk_scrolled_window_new();
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), 
                                     GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_window_set_child(GTK_WINDOW(m_window), scrolled_window);

        // Create main container
        GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
        gtk_widget_set_margin_top(main_box, 20);
        gtk_widget_set_margin_bottom(main_box, 20);
        gtk_widget_set_margin_start(main_box, 20);
        gtk_widget_set_margin_end(main_box, 20);
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), main_box);

        // Create header
        createHeaderSection(main_box);
        
        // Create status section
        createStatusSection(main_box);
        
        // Create system information section
        createInfoSection(main_box);
        
        // Create detailed report section
        createReportSection(main_box);
        
        // Create instructions section
        createInstructionsSection(main_box);
        
        // Create refresh button
        createRefreshButton(main_box);

        // Load and display initial hardware info
        updateHardwareInfoDisplay();
        
        // Show window
        gtk_window_present(GTK_WINDOW(m_window));
        
        std::cout << "GTK4 Hardware Acceleration Test started!" << std::endl;
    }

    void loadCustomCSS() {
        GtkCssProvider *css_provider = gtk_css_provider_new();
        const char *css_data = 
            ".header-label { font-size: 24px; font-weight: bold; margin-bottom: 20px; }"
            ".status-frame-enabled { background-color: #4CAF50; border-radius: 8px; padding: 15px; }"
            ".status-frame-disabled { background-color: #F44336; border-radius: 8px; padding: 15px; }"
            ".status-title { font-size: 18px; font-weight: bold; color: white; margin-bottom: 10px; }"
            ".status-text { font-size: 16px; font-weight: bold; color: white; }"
            ".info-frame { background-color: #E0E0E0; border-radius: 8px; padding: 15px; }"
            ".info-title { font-size: 18px; font-weight: bold; margin-bottom: 10px; }"
            ".info-text { font-family: monospace; font-size: 12px; }"
            ".report-button { background-color: #2196F3; color: white; border-radius: 4px; "
            "padding: 12px; font-weight: bold; }"
            ".report-frame { background-color: #F5F5F5; border-radius: 4px; padding: 10px; }"
            ".instructions-frame { background-color: #E3F2FD; border-radius: 8px; padding: 15px; }"
            ".instructions-title { font-size: 16px; font-weight: bold; margin-bottom: 10px; }"
            ".instructions-text { font-size: 12px; }"
            ".refresh-button { font-weight: bold; padding: 10px; }";
        
        gtk_css_provider_load_from_data(css_provider, css_data, -1);
        gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                                   GTK_STYLE_PROVIDER(css_provider),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    void createHeaderSection(GtkWidget *main_box) {
        GtkWidget *header_label = gtk_label_new("Welcome to GTK4 Hardware Acceleration Test with Torizon!");
        gtk_widget_add_css_class(header_label, "header-label");
        gtk_label_set_justify(GTK_LABEL(header_label), GTK_JUSTIFY_CENTER);
        gtk_label_set_wrap(GTK_LABEL(header_label), TRUE);
        gtk_box_append(GTK_BOX(main_box), header_label);
    }

    void createStatusSection(GtkWidget *main_box) {
        // Create frame that will be styled based on hardware acceleration status
        m_status_frame = gtk_frame_new(NULL);
        gtk_box_append(GTK_BOX(main_box), m_status_frame);

        GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_frame_set_child(GTK_FRAME(m_status_frame), status_box);

        m_status_title_label = gtk_label_new("Hardware Acceleration Status");
        gtk_widget_add_css_class(m_status_title_label, "status-title");
        gtk_label_set_justify(GTK_LABEL(m_status_title_label), GTK_JUSTIFY_CENTER);
        gtk_box_append(GTK_BOX(status_box), m_status_title_label);

        m_status_text_label = gtk_label_new("Checking...");
        gtk_widget_add_css_class(m_status_text_label, "status-text");
        gtk_label_set_justify(GTK_LABEL(m_status_text_label), GTK_JUSTIFY_CENTER);
        gtk_label_set_wrap(GTK_LABEL(m_status_text_label), TRUE);
        gtk_box_append(GTK_BOX(status_box), m_status_text_label);
    }

    void createInfoSection(GtkWidget *main_box) {
        GtkWidget *info_frame = gtk_frame_new(NULL);
        gtk_widget_add_css_class(info_frame, "info-frame");
        gtk_box_append(GTK_BOX(main_box), info_frame);

        GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_frame_set_child(GTK_FRAME(info_frame), info_box);

        GtkWidget *info_title = gtk_label_new("System Information");
        gtk_widget_add_css_class(info_title, "info-title");
        gtk_box_append(GTK_BOX(info_box), info_title);

        m_info_label = gtk_label_new("Loading hardware information...");
        gtk_widget_add_css_class(m_info_label, "info-text");
        gtk_label_set_wrap(GTK_LABEL(m_info_label), TRUE);
        gtk_label_set_selectable(GTK_LABEL(m_info_label), TRUE);
        gtk_widget_set_halign(m_info_label, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(info_box), m_info_label);
    }

    void createReportSection(GtkWidget *main_box) {
        GtkWidget *report_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_box_append(GTK_BOX(main_box), report_box);

        // Toggle button for detailed report
        m_report_toggle_button = gtk_button_new_with_label("▶ Detailed Hardware Report");
        gtk_widget_add_css_class(m_report_toggle_button, "report-button");
        g_signal_connect(m_report_toggle_button, "clicked", G_CALLBACK(toggle_detailed_report), this);
        gtk_box_append(GTK_BOX(report_box), m_report_toggle_button);

        // Report content frame (initially hidden)
        m_report_content_frame = gtk_frame_new(NULL);
        gtk_widget_add_css_class(m_report_content_frame, "report-frame");
        gtk_widget_set_visible(m_report_content_frame, FALSE);
        gtk_box_append(GTK_BOX(report_box), m_report_content_frame);

        m_report_text_label = gtk_label_new("Click above to see detailed report");
        gtk_widget_add_css_class(m_report_text_label, "info-text");
        gtk_label_set_wrap(GTK_LABEL(m_report_text_label), TRUE);
        gtk_label_set_selectable(GTK_LABEL(m_report_text_label), TRUE);
        gtk_widget_set_halign(m_report_text_label, GTK_ALIGN_START);
        gtk_frame_set_child(GTK_FRAME(m_report_content_frame), m_report_text_label);
    }

    void createInstructionsSection(GtkWidget *main_box) {
        GtkWidget *instructions_frame = gtk_frame_new(NULL);
        gtk_widget_add_css_class(instructions_frame, "instructions-frame");
        gtk_box_append(GTK_BOX(main_box), instructions_frame);

        GtkWidget *instructions_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_frame_set_child(GTK_FRAME(instructions_frame), instructions_box);

        GtkWidget *instructions_title = gtk_label_new("Instructions");
        gtk_widget_add_css_class(instructions_title, "instructions-title");
        gtk_box_append(GTK_BOX(instructions_box), instructions_title);

        GtkWidget *instructions_text = gtk_label_new(
            "This application tests hardware acceleration capabilities on your system. "
            "Green status indicates hardware acceleration is working. "
            "Red status indicates software rendering is being used.\n\n"
            "For optimal performance on embedded systems, ensure:\n"
            "• GPU drivers are properly installed\n"
            "• EGL libraries are available (Linux)\n"
            "• Hardware acceleration is enabled in system settings"
        );
        gtk_widget_add_css_class(instructions_text, "instructions-text");
        gtk_label_set_wrap(GTK_LABEL(instructions_text), TRUE);
        gtk_widget_set_halign(instructions_text, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(instructions_box), instructions_text);
    }

    void createRefreshButton(GtkWidget *main_box) {
        m_refresh_button = gtk_button_new_with_label("Refresh Hardware Information");
        gtk_widget_add_css_class(m_refresh_button, "refresh-button");
        gtk_widget_add_css_class(m_refresh_button, "suggested-action");
        gtk_widget_set_halign(m_refresh_button, GTK_ALIGN_CENTER);
        g_signal_connect(m_refresh_button, "clicked", G_CALLBACK(refresh_button_clicked), this);
        gtk_box_append(GTK_BOX(main_box), m_refresh_button);
    }

    void refreshHardwareInfo() {
        gtk_button_set_label(GTK_BUTTON(m_refresh_button), "Refreshing...");
        gtk_widget_set_sensitive(m_refresh_button, FALSE);
        
        // Refresh synchronously for now (could be made async)
        m_hardwareService->refreshHardwareInfo();
        updateHardwareInfoDisplay();
        
        gtk_button_set_label(GTK_BUTTON(m_refresh_button), "Refresh Hardware Information");
        gtk_widget_set_sensitive(m_refresh_button, TRUE);
    }

    void updateHardwareInfoDisplay() {
        // Update status section with color coding
        if (m_hardwareService->isHardwareAccelerated()) {
            gtk_widget_remove_css_class(m_status_frame, "status-frame-disabled");
            gtk_widget_add_css_class(m_status_frame, "status-frame-enabled");
            gtk_label_set_text(GTK_LABEL(m_status_text_label), "✅ Hardware Acceleration: ENABLED");
        } else {
            gtk_widget_remove_css_class(m_status_frame, "status-frame-enabled");
            gtk_widget_add_css_class(m_status_frame, "status-frame-disabled");
            gtk_label_set_text(GTK_LABEL(m_status_text_label), "❌ Hardware Acceleration: DISABLED (Software Rendering)");
        }

        // Update system information
        std::string info_text = m_hardwareService->getHardwareInfoText();
        gtk_label_set_text(GTK_LABEL(m_info_label), info_text.c_str());

        // Update detailed report
        std::string report_text = m_hardwareService->getDetailedReport();
        gtk_label_set_text(GTK_LABEL(m_report_text_label), report_text.c_str());
    }

    void toggleDetailedReport() {
        m_report_expanded = !m_report_expanded;
        gtk_widget_set_visible(m_report_content_frame, m_report_expanded);
        
        const char* label = m_report_expanded ? "▼ Detailed Hardware Report" : "▶ Detailed Hardware Report";
        gtk_button_set_label(GTK_BUTTON(m_report_toggle_button), label);
    }

private:
    std::unique_ptr<HardwareAccelerationService> m_hardwareService;
    GtkWidget *m_window = nullptr;
    GtkWidget *m_status_frame = nullptr;
    GtkWidget *m_status_title_label = nullptr;
    GtkWidget *m_status_text_label = nullptr;
    GtkWidget *m_info_label = nullptr;
    GtkWidget *m_report_toggle_button = nullptr;
    GtkWidget *m_report_content_frame = nullptr;
    GtkWidget *m_report_text_label = nullptr;
    GtkWidget *m_refresh_button = nullptr;
    bool m_report_expanded = false;
};

int main(int argc, char **argv) {
    HardwareAccelerationApp app;
    app.run(argc, argv);
    return 0;
}
