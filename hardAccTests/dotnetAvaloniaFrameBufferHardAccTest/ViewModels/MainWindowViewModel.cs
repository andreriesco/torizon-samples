using dotnetAvaloniaHATest.Services;
using ReactiveUI;
using System.Threading.Tasks;
using Avalonia.Data.Converters;
using Avalonia.Media;
using System;
using System.Globalization;

namespace dotnetAvaloniaHATest.ViewModels;

public class MainWindowViewModel : ViewModelBase
{
    private string _greeting = "Welcome to Avalonia with Torizon!";
    private string _hardwareInfo = "Loading hardware information...";
    private bool _isHardwareAccelerated;
    private string _detailedReport = "";

    public string Greeting
    {
        get => _greeting;
        set => this.RaiseAndSetIfChanged(ref _greeting, value);
    }

    public string HardwareInfo
    {
        get => _hardwareInfo;
        set => this.RaiseAndSetIfChanged(ref _hardwareInfo, value);
    }

    public bool IsHardwareAccelerated
    {
        get => _isHardwareAccelerated;
        set => this.RaiseAndSetIfChanged(ref _isHardwareAccelerated, value);
    }

    public string DetailedReport
    {
        get => _detailedReport;
        set => this.RaiseAndSetIfChanged(ref _detailedReport, value);
    }

    public static readonly IValueConverter AccelerationStatusConverter = 
        new FuncValueConverter<bool, IBrush>(isAccelerated => 
            isAccelerated ? new SolidColorBrush(Colors.Green) : new SolidColorBrush(Colors.Red));

    public static readonly IValueConverter AccelerationTextConverter = 
        new FuncValueConverter<bool, string>(isAccelerated => 
            isAccelerated ? "✅ Hardware Acceleration: ENABLED" : "❌ Hardware Acceleration: DISABLED (Software Rendering)");

    public MainWindowViewModel()
    {
        _ = LoadHardwareInfoAsync(); // Fire and forget, but suppress warning
    }

    private async Task LoadHardwareInfoAsync()
    {
        await Task.Run(() =>
        {
            var info = HardwareAccelerationService.GetHardwareAccelerationInfo();
            
            // Print detailed hardware acceleration log
            Console.WriteLine("\n=== .NET Avalonia FrameBuffer Hardware Acceleration Detailed Log ===");
            Console.WriteLine("Project: dotnetAvaloniaFrameBufferHardAccTest");
            Console.WriteLine("Framework: Avalonia + .NET (FrameBuffer)");
            Console.WriteLine($"Backend: {info.RenderingBackend}");
            Console.WriteLine($"Hardware Accelerated: {(info.IsHardwareAccelerated ? "Yes" : "No")}");
            Console.WriteLine($"\n{HardwareAccelerationService.GetDetailedReport()}");
            Console.WriteLine("=== End of Detailed Log ===\n");
            
            HardwareInfo = $"Platform: {info.Platform}\n" +
                          $"Hardware Accelerated: {(info.IsHardwareAccelerated ? "✅ Yes" : "❌ No")}\n" +
                          $"Rendering Backend: {info.RenderingBackend}\n" +
                          $"Graphics Card: {info.GraphicsCard}\n" +
                          $"OpenGL Renderer: {info.OpenGLRenderer}";
            
            IsHardwareAccelerated = info.IsHardwareAccelerated;
            DetailedReport = HardwareAccelerationService.GetDetailedReport();
        });
    }
}
