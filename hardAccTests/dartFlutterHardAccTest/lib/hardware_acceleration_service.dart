import 'dart:io';
import 'dart:async';

class HardwareAccelerationInfo {
  final bool isHardwareAccelerated;
  final String renderingBackend;
  final String openGLVersion;
  final String openGLVendor;
  final String openGLRenderer;
  final String graphicsCard;
  final List<String> supportedExtensions;
  final bool isEglSupported;
  final bool isSoftwareRendering;
  final String platform;
  final String surfaceFormat;

  const HardwareAccelerationInfo({
    this.isHardwareAccelerated = false,
    this.renderingBackend = 'Unknown',
    this.openGLVersion = 'Unknown',
    this.openGLVendor = 'Unknown',
    this.openGLRenderer = 'Unknown',
    this.graphicsCard = 'Unknown',
    this.supportedExtensions = const [],
    this.isEglSupported = false,
    this.isSoftwareRendering = false,
    this.platform = 'Unknown',
    this.surfaceFormat = 'Unknown',
  });

  HardwareAccelerationInfo copyWith({
    bool? isHardwareAccelerated,
    String? renderingBackend,
    String? openGLVersion,
    String? openGLVendor,
    String? openGLRenderer,
    String? graphicsCard,
    List<String>? supportedExtensions,
    bool? isEglSupported,
    bool? isSoftwareRendering,
    String? platform,
    String? surfaceFormat,
  }) {
    return HardwareAccelerationInfo(
      isHardwareAccelerated: isHardwareAccelerated ?? this.isHardwareAccelerated,
      renderingBackend: renderingBackend ?? this.renderingBackend,
      openGLVersion: openGLVersion ?? this.openGLVersion,
      openGLVendor: openGLVendor ?? this.openGLVendor,
      openGLRenderer: openGLRenderer ?? this.openGLRenderer,
      graphicsCard: graphicsCard ?? this.graphicsCard,
      supportedExtensions: supportedExtensions ?? this.supportedExtensions,
      isEglSupported: isEglSupported ?? this.isEglSupported,
      isSoftwareRendering: isSoftwareRendering ?? this.isSoftwareRendering,
      platform: platform ?? this.platform,
      surfaceFormat: surfaceFormat ?? this.surfaceFormat,
    );
  }
}

class HardwareAccelerationService {
  static HardwareAccelerationInfo? _cachedInfo;
  
  static Future<HardwareAccelerationInfo> getHardwareAccelerationInfo() async {
    if (_cachedInfo != null) {
      return _cachedInfo!;
    }

    final info = await _loadHardwareInfo();
    _cachedInfo = info;
    return info;
  }

  static Future<HardwareAccelerationInfo> _loadHardwareInfo() async {
    try {
      // Detect platform
      final platform = _getPlatformInfo();
      
      // Get graphics card information
      final graphicsCard = await _getGraphicsCardInfo();
      
      // Get OpenGL information
      final glInfo = await _getOpenGLInfo();
      
      // Determine if hardware accelerated
      final rendererLower = glInfo['renderer']?.toLowerCase() ?? '';
      final isHardwareAccelerated = !rendererLower.contains('software') && 
                                   !rendererLower.contains('llvmpipe') &&
                                   !rendererLower.contains('mesa software');
      
      // Detect rendering backend
      final renderingBackend = _getRenderingBackend();
      
      // Check if software rendering is being used
      final isSoftwareRendering = renderingBackend.contains('Software') || 
                                rendererLower.contains('software') ||
                                rendererLower.contains('llvmpipe') ||
                                rendererLower.contains('mesa software');
      
      // Check EGL support on Linux
      bool isEglSupported = false;
      if (Platform.isLinux) {
        isEglSupported = await _checkEglSupport();
      }

      return HardwareAccelerationInfo(
        platform: platform,
        graphicsCard: graphicsCard,
        openGLVersion: glInfo['version'] ?? 'Unknown',
        openGLVendor: glInfo['vendor'] ?? 'Unknown',
        openGLRenderer: glInfo['renderer'] ?? 'Unknown',
        supportedExtensions: glInfo['extensions'] ?? [],
        surfaceFormat: glInfo['surfaceFormat'] ?? 'Unknown',
        isHardwareAccelerated: isHardwareAccelerated,
        renderingBackend: renderingBackend,
        isSoftwareRendering: isSoftwareRendering,
        isEglSupported: isEglSupported,
      );
    } catch (e) {
      print('Error getting hardware acceleration info: $e');
      return const HardwareAccelerationInfo(
        platform: 'Error detecting platform',
        graphicsCard: 'Error detecting graphics card',
        openGLRenderer: 'Error detecting OpenGL',
      );
    }
  }

  static String _getPlatformInfo() {
    if (Platform.isWindows) return 'Windows';
    if (Platform.isLinux) return 'Linux';
    if (Platform.isMacOS) return 'macOS';
    if (Platform.isAndroid) return 'Android';
    if (Platform.isIOS) return 'iOS';
    return Platform.operatingSystem;
  }

  static Future<String> _getGraphicsCardInfo() async {
    if (Platform.isLinux) {
      return await _getLinuxGraphicsInfo();
    } else if (Platform.isWindows) {
      return _getWindowsGraphicsInfo();
    }
    return 'Platform not supported for graphics detection';
  }

  static Future<String> _getLinuxGraphicsInfo() async {
    try {
      // First, try alternative methods that don't require external commands
      final graphicsInfo = await _getLinuxGraphicsInfoFromSysfs();
      if (graphicsInfo.isNotEmpty) {
        return graphicsInfo;
      }

      // Try lspci only if available and with timeout
      if (await _isCommandAvailable('lspci')) {
        try {
          final result = await Process.run('lspci', ['-nn'])
              .timeout(const Duration(seconds: 3));
          if (result.exitCode == 0) {
            final lines = result.stdout.toString().split('\n');
            for (final line in lines) {
              if (line.toLowerCase().contains('vga') || line.toLowerCase().contains('3d')) {
                return line.trim();
              }
            }
          }
        } catch (e) {
          print('Error running lspci: $e');
          // Continue to fallback methods
        }
      }

      // Fallback methods
      try {
        if (await File('/proc/driver/nvidia/version').exists()) {
          return 'NVIDIA GPU (driver detected)';
        }
      } catch (e) {
        print('Error checking NVIDIA driver: $e');
      }
      
      // Check for AMD GPU
      try {
        if (await Directory('/sys/class/drm').exists()) {
          final drmDir = Directory('/sys/class/drm');
          final cards = await drmDir.list().where((entity) => 
            entity is Directory && entity.path.contains('card')).toList();
          if (cards.isNotEmpty) {
            return 'GPU detected via DRM';
          }
        }
      } catch (e) {
        print('Error checking DRM directory: $e');
      }
    } catch (e) {
      print('Error getting Linux graphics info: $e');
    }

    return 'Graphics hardware detection limited in container environment';
  }

  static Future<String> _getLinuxGraphicsInfoFromSysfs() async {
    try {
      final drmDir = Directory('/sys/class/drm');
      if (!await drmDir.exists()) {
        return '';
      }

      final cards = <String>[];
      await for (final entity in drmDir.list()) {
        if (entity is Directory) {
          final cardName = entity.uri.pathSegments.last;
          // Filter out card*-* entries, keep only card[0-9]+
          if (RegExp(r'^card\d+$').hasMatch(cardName)) {
            cards.add(cardName);
          }
        }
      }

      if (cards.isEmpty) {
        return '';
      }

      final gpuInfo = <String>[];
      
      for (final card in cards) {
        final devicePath = '/sys/class/drm/$card/device';
        
        // Try to read vendor and device info
        final vendorFile = File('$devicePath/vendor');
        final deviceFile = File('$devicePath/device');

        if (await vendorFile.exists() && await deviceFile.exists()) {
          try {
            final vendor = (await vendorFile.readAsString()).trim();
            final device = (await deviceFile.readAsString()).trim();
            
            final vendorName = _getVendorName(vendor);
            gpuInfo.add('$vendorName GPU ($card) - Vendor: $vendor, Device: $device');
          } catch (e) {
            print('Error reading vendor/device info for $card: $e');
          }
        }
      }
      
      if (gpuInfo.isNotEmpty) {
        return gpuInfo.join('; ');
      }
      
      return 'Graphics devices detected: ${cards.join(', ')}';
    } catch (e) {
      print('Error reading from sysfs: $e');
      return '';
    }
  }

  static String _getWindowsGraphicsInfo() {
    return 'Windows Graphics Device';
  }

  static String _getRenderingBackend() {
    if (Platform.isLinux) {
      // Check if we're running in a framebuffer environment
      final displayEnv = Platform.environment['DISPLAY'];
      final waylandDisplay = Platform.environment['WAYLAND_DISPLAY'];
      
      if ((displayEnv == null || displayEnv.isEmpty) && 
          (waylandDisplay == null || waylandDisplay.isEmpty)) {
        // Likely framebuffer mode
        return 'Linux Framebuffer';
      } else if (waylandDisplay != null && waylandDisplay.isNotEmpty) {
        return 'Wayland';
      } else if (displayEnv != null && displayEnv.isNotEmpty) {
        return 'X11';
      }
      
      return 'Linux (Display Server Unknown)';
    } else if (Platform.isWindows) {
      return 'Win32';
    } else if (Platform.isMacOS) {
      return 'macOS';
    } else if (Platform.isAndroid) {
      return 'Android';
    } else if (Platform.isIOS) {
      return 'iOS';
    }
    
    return 'Unknown';
  }

  static Future<bool> _checkEglSupport() async {
    try {
      // Check if EGL libraries are available in common locations
      final eglPaths = [
        '/usr/lib/x86_64-linux-gnu/libEGL.so.1',
        '/usr/lib/x86_64-linux-gnu/libEGL.so',
        '/usr/lib/aarch64-linux-gnu/libEGL.so.1',
        '/usr/lib/aarch64-linux-gnu/libEGL.so',
        '/usr/lib/arm-linux-gnueabihf/libEGL.so.1',
        '/usr/lib/arm-linux-gnueabihf/libEGL.so',
        '/usr/lib/libEGL.so.1',
        '/usr/lib/libEGL.so',
        '/usr/lib64/libEGL.so.1',
        '/usr/lib64/libEGL.so',
        '/lib/x86_64-linux-gnu/libEGL.so.1',
        '/lib/x86_64-linux-gnu/libEGL.so',
        '/lib/aarch64-linux-gnu/libEGL.so.1',
        '/lib/aarch64-linux-gnu/libEGL.so',
        '/opt/vc/lib/libEGL.so', // Raspberry Pi
        '/usr/local/lib/libEGL.so',
      ];

      for (final path in eglPaths) {
        if (await File(path).exists()) {
          print('Found EGL library at: $path');
          return true;
        }
      }

      // Check LD_LIBRARY_PATH for EGL libraries
      final ldLibraryPath = Platform.environment['LD_LIBRARY_PATH'];
      if (ldLibraryPath != null && ldLibraryPath.isNotEmpty) {
        final paths = ldLibraryPath.split(':');
        for (final path in paths) {
          final dir = Directory(path);
          if (await dir.exists()) {
            await for (final entity in dir.list()) {
              if (entity is File && 
                  entity.uri.pathSegments.last.startsWith('libEGL.so')) {
                print('Found EGL library in LD_LIBRARY_PATH: ${entity.path}');
                return true;
              }
            }
          }
        }
      }

      // Try to run eglinfo if available (but don't fail if missing)
      if (await _isCommandAvailable('eglinfo')) {
        try {
          final result = await Process.run('eglinfo', [])
              .timeout(const Duration(seconds: 3));
          if (result.exitCode == 0) {
            print('eglinfo command executed successfully');
            return true;
          }
        } catch (e) {
          print('Error running eglinfo: $e');
        }
      }
    } catch (e) {
      print('Error checking EGL support: $e');
    }

    return false;
  }

  static Future<bool> _isCommandAvailable(String command) async {
    try {
      // Add timeout to prevent hanging
      final result = await Process.run('which', [command])
          .timeout(const Duration(seconds: 2));
      return result.exitCode == 0;
    } catch (e) {
      // If 'which' fails, assume command is not available
      print('Command availability check failed for $command: $e');
      return false;
    }
  }

  static String _getVendorName(String vendorId) {
    final vid = vendorId.toLowerCase();
    switch (vid) {
      case '0x1002': return 'AMD';
      case '0x10de': return 'NVIDIA';
      case '0x8086': return 'Intel';
      case '0x1234': return 'QEMU';
      case '0x15ad': return 'VMware';
      default: return vendorId;
    }
  }

  static Future<Map<String, dynamic>> _getOpenGLInfo() async {
    final result = <String, dynamic>{
      'version': 'OpenGL detection requires runtime context',
      'vendor': 'Unknown',
      'renderer': 'Unknown',
      'extensions': <String>[],
      'surfaceFormat': 'Not available without GL context',
    };
    
    // On Linux, we can try to get some basic GL info from glxinfo if available
    if (Platform.isLinux && await _isCommandAvailable('glxinfo')) {
      try {
        final processResult = await Process.run('glxinfo', [])
            .timeout(const Duration(seconds: 5));
        if (processResult.exitCode == 0) {
          final lines = processResult.stdout.toString().split('\n');
          
          for (final line in lines) {
            if (line.contains('OpenGL vendor string:')) {
              final parts = line.split(':');
              if (parts.length > 1) {
                result['vendor'] = parts[1].trim();
              }
            } else if (line.contains('OpenGL renderer string:')) {
              final parts = line.split(':');
              if (parts.length > 1) {
                result['renderer'] = parts[1].trim();
              }
            } else if (line.contains('OpenGL version string:')) {
              final parts = line.split(':');
              if (parts.length > 1) {
                result['version'] = parts[1].trim();
              }
            }
          }
        }
      } catch (e) {
        print('Error running glxinfo: $e');
      }
    }
    
    // If glxinfo didn't work, try alternative methods
    if (result['renderer'] == 'Unknown') {
      // Check if we're in a software rendering environment
      if (await File('/usr/lib/x86_64-linux-gnu/dri/swrast_dri.so').exists() ||
          await File('/usr/lib/dri/swrast_dri.so').exists()) {
        result['renderer'] = 'Software Rasterizer (Mesa)';
      }
    }
    
    return result;
  }

  static String getHardwareInfoText(HardwareAccelerationInfo info) {
    return 'Platform: ${info.platform}\n'
           'Hardware Accelerated: ${info.isHardwareAccelerated ? "✅ Yes" : "❌ No"}\n'
           'Rendering Backend: ${info.renderingBackend}\n'
           'Graphics Card: ${info.graphicsCard}\n'
           'OpenGL Renderer: ${info.openGLRenderer}';
  }

  static String getDetailedReport(HardwareAccelerationInfo info) {
    final buffer = StringBuffer();
    buffer.writeln('=== Hardware Acceleration Report ===');
    buffer.writeln('Platform: ${info.platform}');
    buffer.writeln('Hardware Accelerated: ${info.isHardwareAccelerated ? "Yes" : "No"}');
    buffer.writeln('Software Rendering: ${info.isSoftwareRendering ? "Yes" : "No"}');
    buffer.writeln('Rendering Backend: ${info.renderingBackend}');
    buffer.writeln('Graphics Card: ${info.graphicsCard}');
    buffer.writeln('OpenGL Version: ${info.openGLVersion}');
    buffer.writeln('OpenGL Vendor: ${info.openGLVendor}');
    buffer.writeln('OpenGL Renderer: ${info.openGLRenderer}');
    
    if (info.surfaceFormat.isNotEmpty && info.surfaceFormat != 'Unknown') {
      buffer.writeln('Surface Format: ${info.surfaceFormat}');
    }
    
    if (Platform.isLinux) {
      buffer.writeln('EGL Support: ${info.isEglSupported ? "Yes" : "No"}');
    }
    
    if (info.supportedExtensions.isNotEmpty && info.supportedExtensions.length < 50) {
      buffer.writeln('OpenGL Extensions: ${info.supportedExtensions.join(", ")}');
    } else if (info.supportedExtensions.isNotEmpty) {
      buffer.writeln('OpenGL Extensions: ${info.supportedExtensions.length} extensions available');
    }
    
    return buffer.toString();
  }

  static void clearCache() {
    _cachedInfo = null;
  }
}