/*******************************************************************
 *
 * main.c - LVGL Hardware Acceleration Test Application
 *
 * Based on lv_port_linux/main.c and adapted for hardware acceleration testing
 *
 * Copyright (c) 2024 LVGL LLC.
 *
 * Authors:
 * LVGL LLC, gabriel.catel@edgemtech.ch
 * LVGL LLC, erik.tagirov@edgemtech.ch
 * Hardware Acceleration Test adaptation
 *
 ******************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "hardware_acceleration_service.h"

#define FRAME_BUFFER_DEV "/dev/fb0"

// Fixed display resolution
#define DISPLAY_WIDTH  1920
#define DISPLAY_HEIGHT 1080

// Global variables
static hardware_acceleration_service_t hardware_service;
static lv_obj_t *main_screen;
static lv_obj_t *status_label;
static lv_obj_t *status_panel;
static lv_obj_t *info_label;
static lv_obj_t *report_panel;
static lv_obj_t *report_label;
static lv_obj_t *report_toggle_btn;
static lv_obj_t *refresh_btn;
static bool report_expanded = false;

// Function prototypes
static void create_hardware_acceleration_ui(void);
static void create_header_section(lv_obj_t *parent);
static void create_status_section(lv_obj_t *parent);
static void create_info_section(lv_obj_t *parent);
static void create_report_section(lv_obj_t *parent);
static void create_instructions_section(lv_obj_t *parent);
static void create_refresh_button(lv_obj_t *parent);
static void update_display(void);
static void refresh_button_clicked(lv_event_t *e);
static void toggle_report_clicked(lv_event_t *e);

static void lv_linux_disp_init(void)
{
    const char *device = FRAME_BUFFER_DEV;
    lv_display_t * disp = lv_linux_fbdev_create();

    lv_linux_fbdev_set_file(disp, device);
    
    // Set the display resolution
    lv_display_set_resolution(disp, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    // Configure the display for optimal rendering
    lv_display_set_antialiasing(disp, true);
}

static void lv_linux_run_loop(void)
{
    uint32_t time_till_next;

    /*Handle LVGL tasks*/
    while(1) {
        time_till_next = lv_timer_handler();
        usleep(time_till_next);
    }
}

static void create_hardware_acceleration_ui(void)
{
    // Create main screen
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0xF5F5F5), 0);
    
    // Create scrollable container optimized for the display resolution
    lv_obj_t *scroll_container = lv_obj_create(main_screen);
    lv_obj_set_size(scroll_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_width(scroll_container, 0, 0);
    lv_obj_set_style_pad_all(scroll_container, 20, 0);
    lv_obj_set_flex_flow(scroll_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scroll_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create UI sections
    create_header_section(scroll_container);
    create_status_section(scroll_container);
    create_info_section(scroll_container);
    create_report_section(scroll_container);
    create_instructions_section(scroll_container);
    create_refresh_button(scroll_container);
    
    // Load screen
    lv_screen_load(main_screen);
    
    // Update display with initial data
    update_display();
}

static void create_header_section(lv_obj_t *parent)
{
    lv_obj_t *header_label = lv_label_create(parent);
    lv_label_set_text(header_label, "Welcome to LVGL Hardware Acceleration Test with Torizon!");
    lv_obj_set_style_text_font(header_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(header_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(header_label, LV_PCT(90));
    lv_obj_set_style_margin_bottom(header_label, 20, 0);
    lv_label_set_long_mode(header_label, LV_LABEL_LONG_WRAP);
}

static void create_status_section(lv_obj_t *parent)
{
    // Create status panel
    status_panel = lv_obj_create(parent);
    lv_obj_set_size(status_panel, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(status_panel, 0, 0);
    lv_obj_set_style_radius(status_panel, 8, 0);
    lv_obj_set_style_pad_all(status_panel, 15, 0);
    lv_obj_set_style_margin_bottom(status_panel, 20, 0);
    lv_obj_set_flex_flow(status_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Status title
    lv_obj_t *status_title = lv_label_create(status_panel);
    lv_label_set_text(status_title, "Hardware Acceleration Status");
    lv_obj_set_style_text_font(status_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_margin_bottom(status_title, 10, 0);
    
    // Status text
    status_label = lv_label_create(status_panel);
    lv_label_set_text(status_label, "Checking...");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(status_label, LV_PCT(100));
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
}

static void create_info_section(lv_obj_t *parent)
{
    // Create info panel
    lv_obj_t *info_panel = lv_obj_create(parent);
    lv_obj_set_size(info_panel, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(info_panel, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_border_width(info_panel, 0, 0);
    lv_obj_set_style_radius(info_panel, 8, 0);
    lv_obj_set_style_pad_all(info_panel, 15, 0);
    lv_obj_set_style_margin_bottom(info_panel, 20, 0);
    lv_obj_set_flex_flow(info_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    
    // Info title
    lv_obj_t *info_title = lv_label_create(info_panel);
    lv_label_set_text(info_title, "System Information");
    lv_obj_set_style_text_font(info_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(info_title, lv_color_hex(0x000000), 0);
    lv_obj_set_style_margin_bottom(info_title, 10, 0);
    
    // Info text
    info_label = lv_label_create(info_panel);
    lv_label_set_text(info_label, "Loading hardware information...");
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(info_label, lv_color_hex(0x000000), 0);
    lv_obj_set_width(info_label, LV_PCT(100));
    lv_label_set_long_mode(info_label, LV_LABEL_LONG_WRAP);
}

static void create_report_section(lv_obj_t *parent)
{
    // Create report container
    lv_obj_t *report_container = lv_obj_create(parent);
    lv_obj_set_size(report_container, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(report_container, 0, 0);
    lv_obj_set_style_bg_opa(report_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(report_container, 0, 0);
    lv_obj_set_style_margin_bottom(report_container, 20, 0);
    lv_obj_set_flex_flow(report_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(report_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Toggle button
    report_toggle_btn = lv_button_create(report_container);
    lv_obj_set_size(report_toggle_btn, LV_PCT(100), 50);
    lv_obj_set_style_bg_color(report_toggle_btn, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(report_toggle_btn, 0, 0);
    lv_obj_set_style_radius(report_toggle_btn, 4, 0);
    lv_obj_set_style_margin_bottom(report_toggle_btn, 10, 0);
    lv_obj_add_event_cb(report_toggle_btn, toggle_report_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_label = lv_label_create(report_toggle_btn);
    lv_label_set_text(btn_label, LV_SYMBOL_RIGHT " Detailed Hardware Report");
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_label);
    
    // Report panel (initially hidden)
    report_panel = lv_obj_create(report_container);
    lv_obj_set_size(report_panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(report_panel, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_border_width(report_panel, 0, 0);
    lv_obj_set_style_radius(report_panel, 4, 0);
    lv_obj_set_style_pad_all(report_panel, 10, 0);
    lv_obj_add_flag(report_panel, LV_OBJ_FLAG_HIDDEN);
    
    report_label = lv_label_create(report_panel);
    lv_label_set_text(report_label, "Click above to see detailed report");
    lv_obj_set_style_text_font(report_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(report_label, lv_color_hex(0x000000), 0);
    lv_obj_set_width(report_label, LV_PCT(100));
    lv_label_set_long_mode(report_label, LV_LABEL_LONG_WRAP);
}

static void create_instructions_section(lv_obj_t *parent)
{
    // Create instructions panel
    lv_obj_t *instructions_panel = lv_obj_create(parent);
    lv_obj_set_size(instructions_panel, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(instructions_panel, lv_color_hex(0xE3F2FD), 0);
    lv_obj_set_style_border_width(instructions_panel, 0, 0);
    lv_obj_set_style_radius(instructions_panel, 8, 0);
    lv_obj_set_style_pad_all(instructions_panel, 15, 0);
    lv_obj_set_style_margin_bottom(instructions_panel, 20, 0);
    lv_obj_set_flex_flow(instructions_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(instructions_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    
    // Instructions title
    lv_obj_t *instructions_title = lv_label_create(instructions_panel);
    lv_label_set_text(instructions_title, "Instructions");
    lv_obj_set_style_text_font(instructions_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(instructions_title, lv_color_hex(0x000000), 0);
    lv_obj_set_style_margin_bottom(instructions_title, 10, 0);
    
    // Instructions text
    lv_obj_t *instructions_text = lv_label_create(instructions_panel);
    lv_label_set_text(instructions_text, 
        "This application tests hardware acceleration capabilities on your system. "
        "Green status indicates hardware acceleration is working. "
        "Red status indicates software rendering is being used.\n\n"
        "For optimal performance on embedded systems, ensure:\n"
        "• GPU drivers are properly installed\n"
        "• EGL libraries are available (Linux)\n"
        "• Hardware acceleration is enabled in system settings");
    lv_obj_set_style_text_font(instructions_text, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(instructions_text, lv_color_hex(0x000000), 0);
    lv_obj_set_width(instructions_text, LV_PCT(100));
    lv_label_set_long_mode(instructions_text, LV_LABEL_LONG_WRAP);
}

static void create_refresh_button(lv_obj_t *parent)
{
    refresh_btn = lv_button_create(parent);
    lv_obj_set_size(refresh_btn, 250, 50);
    lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_border_width(refresh_btn, 0, 0);
    lv_obj_set_style_radius(refresh_btn, 4, 0);
    lv_obj_set_style_margin_top(refresh_btn, 10, 0);
    lv_obj_add_event_cb(refresh_btn, refresh_button_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_label = lv_label_create(refresh_btn);
    lv_label_set_text(btn_label, "Refresh Hardware Information");
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_label);
}

static void update_display(void)
{
    // Update status section with color coding
    if (hardware_acceleration_service_is_accelerated(&hardware_service)) {
        lv_obj_set_style_bg_color(status_panel, lv_color_hex(0x4CAF50), 0);
        lv_label_set_text(status_label, LV_SYMBOL_OK " Hardware Acceleration: ENABLED");
    } else {
        lv_obj_set_style_bg_color(status_panel, lv_color_hex(0xF44336), 0);
        lv_label_set_text(status_label, LV_SYMBOL_CLOSE " Hardware Acceleration: DISABLED (Software Rendering)");
    }
    
    // Update system information
    const char *info_text = hardware_acceleration_service_get_info_text(&hardware_service);
    lv_label_set_text(info_label, info_text);
    
    // Update detailed report
    const char *report_text = hardware_acceleration_service_get_detailed_report(&hardware_service);
    lv_label_set_text(report_label, report_text);
}

static void refresh_button_clicked(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    
    lv_label_set_text(label, "Refreshing...");
    lv_obj_add_state(btn, LV_STATE_DISABLED);
    
    // Refresh hardware info
    hardware_acceleration_service_refresh(&hardware_service);
    update_display();
    
    lv_label_set_text(label, "Refresh Hardware Information");
    lv_obj_remove_state(btn, LV_STATE_DISABLED);
}

static void toggle_report_clicked(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    
    report_expanded = !report_expanded;
    
    if (report_expanded) {
        lv_obj_remove_flag(report_panel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(label, LV_SYMBOL_DOWN " Detailed Hardware Report");
    } else {
        lv_obj_add_flag(report_panel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(label, LV_SYMBOL_RIGHT " Detailed Hardware Report");
    }
}

int main(int argc, char **argv)
{
    /* Initialize LVGL */
    lv_init();

    /* Initialize the configured backend */
    lv_linux_disp_init();

    /* Initialize hardware acceleration service */
    if (hardware_acceleration_service_init(&hardware_service) != 0) {
        printf("Failed to initialize hardware acceleration service\n");
        return -1;
    }

    /* Print detailed hardware acceleration log */
    printf("\n=== C LVGL Hardware Acceleration Detailed Log ===\n");
    printf("Project: cLvglHardAccTest\n");
    printf("Framework: LVGL + C\n");
    printf("Display: Framebuffer (%dx%d)\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    printf("Device: %s\n", FRAME_BUFFER_DEV);
    printf("Hardware Accelerated: %s\n", hardware_acceleration_service_is_accelerated(&hardware_service) ? "Yes" : "No");
    printf("\n%s\n", hardware_acceleration_service_get_detailed_report(&hardware_service));
    printf("=== End of Detailed Log ===\n\n");

    /* Create Hardware Acceleration Test UI */
    create_hardware_acceleration_ui();

    /* Print startup information */
    printf("LVGL Hardware Acceleration Test started\n");
    printf("Display: Framebuffer (%dx%d)\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    printf("Device: %s\n", FRAME_BUFFER_DEV);
    fflush(stdout);

    /* Run the main loop */
    lv_linux_run_loop();

    /* Cleanup */
    hardware_acceleration_service_destroy(&hardware_service);

    return 0;
}
