using Avalonia;
using Avalonia.ReactiveUI;
using Avalonia.Controls.ApplicationLifetimes;
using System.Collections.Generic;
using System;
using System.Diagnostics;

namespace dotnetAvaloniaHATest;

class Program
{
    // Initialization code. Don't use any Avalonia, third-party APIs or any
    // SynchronizationContext-reliant code before AppMain is called: things aren't initialized
    // yet and stuff might break.
    [STAThread]
    public static void Main(string[] args)
    {
        try
        {
            // Log startup information
            Debug.WriteLine("=== Avalonia Hardware Acceleration Test ===");
            Debug.WriteLine($"OS: {Environment.OSVersion}");
            Debug.WriteLine($"Runtime: {Environment.Version}");
            Debug.WriteLine($"64-bit: {Environment.Is64BitProcess}");
            
            BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"Application startup error: {ex.Message}");
            Debug.WriteLine($"Stack trace: {ex.StackTrace}");
            throw;
        }
    }

    // Avalonia configuration, don't remove; also used by visual designer.
    public static AppBuilder BuildAvaloniaApp()
    {
        // Define rendering modes in priority order (EGL first for hardware acceleration)
        IReadOnlyList<X11RenderingMode> renderModes = new List<X11RenderingMode>
        {
            X11RenderingMode.Egl,      // Hardware acceleration via EGL
            X11RenderingMode.Software  // Software fallback
        };

        Debug.WriteLine("Configuring Avalonia with X11 rendering modes:");
        foreach (var mode in renderModes)
        {
            Debug.WriteLine($"  - {mode}");
        }

        var appBuilder = AppBuilder.Configure<App>()
                .UsePlatformDetect()
                .WithInterFont()
                .LogToTrace()  // This will log rendering backend selection
                .UseReactiveUI();

        // Configure X11 platform options for Linux
        if (OperatingSystem.IsLinux())
        {
            Debug.WriteLine("Linux detected - configuring X11 platform options");
            appBuilder = appBuilder.With(new X11PlatformOptions
            {
                RenderingMode = renderModes
            });
        }
        else
        {
            Debug.WriteLine($"Non-Linux platform detected: {Environment.OSVersion.Platform}");
        }

        return appBuilder;
    }
}
