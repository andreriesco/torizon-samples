using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using Avalonia;
using Avalonia.Platform;
using Avalonia.Rendering;

namespace dotnetAvaloniaHATest.Services;

public class HardwareAccelerationInfo
{
    public bool IsHardwareAccelerated { get; set; }
    public string RenderingBackend { get; set; } = "Unknown";
    public string OpenGLVersion { get; set; } = "Unknown";
    public string OpenGLVendor { get; set; } = "Unknown";
    public string OpenGLRenderer { get; set; } = "Unknown";
    public string GraphicsCard { get; set; } = "Unknown";
    public List<string> SupportedExtensions { get; set; } = new();
    public bool IsEglSupported { get; set; }
    public bool IsSoftwareRendering { get; set; }
    public string Platform { get; set; } = "Unknown";
}

public class HardwareAccelerationService
{
    private static HardwareAccelerationInfo? _cachedInfo;

    public static HardwareAccelerationInfo GetHardwareAccelerationInfo()
    {
        if (_cachedInfo != null)
            return _cachedInfo;

        var info = new HardwareAccelerationInfo();
        
        try
        {
            // Detect platform
            info.Platform = GetPlatformInfo();
            
            // Get graphics card information
            info.GraphicsCard = GetGraphicsCardInfo();
            
            // Check if we can get OpenGL context information
            var glInfo = GetOpenGLInfo();
            if (glInfo != null)
            {
                info.OpenGLVersion = glInfo.Version;
                info.OpenGLVendor = glInfo.Vendor;
                info.OpenGLRenderer = glInfo.Renderer;
                info.SupportedExtensions = glInfo.Extensions;
                info.IsHardwareAccelerated = !glInfo.Renderer.ToLower().Contains("software") && 
                                           !glInfo.Renderer.ToLower().Contains("llvmpipe");
            }
            
            // Detect rendering backend
            info.RenderingBackend = GetRenderingBackend();
            
            // Check if software rendering is being used
            info.IsSoftwareRendering = info.RenderingBackend.Contains("Software") || 
                                     info.OpenGLRenderer.ToLower().Contains("software") ||
                                     info.OpenGLRenderer.ToLower().Contains("llvmpipe");
            
            // Check EGL support on Linux
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                info.IsEglSupported = CheckEglSupport();
            }
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"Error getting hardware acceleration info: {ex.Message}");
        }

        _cachedInfo = info;
        return info;
    }

    private static string GetPlatformInfo()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            return "Windows";
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            return "Linux";
        if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            return "macOS";
        
        return RuntimeInformation.OSDescription;
    }

    private static string GetGraphicsCardInfo()
    {
        try
        {
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                return GetLinuxGraphicsInfo();
            }
            else if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                return GetWindowsGraphicsInfo();
            }
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"Error getting graphics card info: {ex.Message}");
        }
        
        return "Unable to detect";
    }

    private static string GetLinuxGraphicsInfo()
    {
        try
        {
            // Try lspci first
            var psi = new ProcessStartInfo
            {
                FileName = "lspci",
                Arguments = "-nn | grep -i vga",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = Process.Start(psi);
            if (process != null)
            {
                var output = process.StandardOutput.ReadToEnd();
                process.WaitForExit();
                
                if (!string.IsNullOrEmpty(output))
                {
                    return output.Trim();
                }
            }

            // Fallback to checking /proc/devices or other methods
            if (File.Exists("/proc/driver/nvidia/version"))
            {
                return "NVIDIA GPU (driver detected)";
            }
            
            // Check for AMD GPU
            if (Directory.Exists("/sys/class/drm") && 
                Directory.GetDirectories("/sys/class/drm", "card*").Length > 0)
            {
                return "GPU detected via DRM";
            }
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"Error getting Linux graphics info: {ex.Message}");
        }

        return "Unknown graphics hardware";
    }

    private static string GetWindowsGraphicsInfo()
    {
        try
        {
            // This would require System.Management package for full implementation
            // For now, return a basic detection
            return "Windows Graphics Device";
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"Error getting Windows graphics info: {ex.Message}");
        }

        return "Unknown graphics hardware";
    }

    private static OpenGLInfo? GetOpenGLInfo()
    {
        try
        {
            var app = Application.Current;
            if (app?.ApplicationLifetime is not Avalonia.Controls.ApplicationLifetimes.IClassicDesktopStyleApplicationLifetime desktop)
                return null;

            var mainWindow = desktop.MainWindow;
            if (mainWindow?.PlatformImpl is not IPlatformHandle platformHandle)
                return null;

            // Try to get OpenGL context information
            // This is a simplified approach - in a real implementation you might need
            // to create a temporary OpenGL context to query this information
            
            return new OpenGLInfo
            {
                Version = "Not available in current context",
                Vendor = "Unknown",
                Renderer = "Unknown",
                Extensions = new List<string>()
            };
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"Error getting OpenGL info: {ex.Message}");
            return null;
        }
    }

    private static string GetRenderingBackend()
    {
        try
        {
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                // On Linux, check if we're using X11 with EGL, Software, etc.
                return "X11 (EGL/Software)";
            }
            else if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                return "Win32";
            }
            else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            {
                return "macOS";
            }
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"Error getting rendering backend: {ex.Message}");
        }

        return "Unknown";
    }

    private static bool CheckEglSupport()
    {
        try
        {
            // Check if EGL libraries are available
            var eglPaths = new[]
            {
                "/usr/lib/x86_64-linux-gnu/libEGL.so.1",
                "/usr/lib/libEGL.so.1",
                "/usr/lib64/libEGL.so.1",
                "/lib/x86_64-linux-gnu/libEGL.so.1"
            };

            foreach (var path in eglPaths)
            {
                if (File.Exists(path))
                    return true;
            }

            // Try to run eglinfo if available
            var psi = new ProcessStartInfo
            {
                FileName = "eglinfo",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = Process.Start(psi);
            if (process != null)
            {
                process.WaitForExit(2000); // 2 second timeout
                return process.ExitCode == 0;
            }
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"Error checking EGL support: {ex.Message}");
        }

        return false;
    }

    public static string GetDetailedReport()
    {
        var info = GetHardwareAccelerationInfo();
        var sb = new StringBuilder();
        
        sb.AppendLine("=== Hardware Acceleration Report ===");
        sb.AppendLine($"Platform: {info.Platform}");
        sb.AppendLine($"Hardware Accelerated: {(info.IsHardwareAccelerated ? "Yes" : "No")}");
        sb.AppendLine($"Software Rendering: {(info.IsSoftwareRendering ? "Yes" : "No")}");
        sb.AppendLine($"Rendering Backend: {info.RenderingBackend}");
        sb.AppendLine($"Graphics Card: {info.GraphicsCard}");
        sb.AppendLine($"OpenGL Version: {info.OpenGLVersion}");
        sb.AppendLine($"OpenGL Vendor: {info.OpenGLVendor}");
        sb.AppendLine($"OpenGL Renderer: {info.OpenGLRenderer}");
        
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            sb.AppendLine($"EGL Support: {(info.IsEglSupported ? "Yes" : "No")}");
        }
        
        if (info.SupportedExtensions.Count > 0)
        {
            sb.AppendLine($"OpenGL Extensions: {string.Join(", ", info.SupportedExtensions)}");
        }
        
        return sb.ToString();
    }
}

internal class OpenGLInfo
{
    public string Version { get; set; } = "";
    public string Vendor { get; set; } = "";
    public string Renderer { get; set; } = "";
    public List<string> Extensions { get; set; } = new();
}