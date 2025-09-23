import 'package:flutter/material.dart';
import 'hardware_acceleration_service.dart';

void main() {
  runApp(const HardwareAccelerationTestApp());
}

class HardwareAccelerationTestApp extends StatelessWidget {
  const HardwareAccelerationTestApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Hardware Acceleration Test',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      home: const MainWindow(),
    );
  }
}

class MainWindow extends StatefulWidget {
  const MainWindow({super.key});

  @override
  State<MainWindow> createState() => _MainWindowState();
}

class _MainWindowState extends State<MainWindow> {
  HardwareAccelerationInfo? _hardwareInfo;
  bool _isLoading = false;
  String? _errorMessage;

  @override
  void initState() {
    super.initState();
    _loadHardwareInfo();
  }

  Future<void> _loadHardwareInfo() async {
    setState(() {
      _isLoading = true;
      _errorMessage = null;
    });

    try {
      final info = await HardwareAccelerationService.getHardwareAccelerationInfo();
      
      // Print detailed hardware acceleration log
      print('\n=== Dart Flutter Hardware Acceleration Detailed Log ===');
      print('Project: dartFlutterHardAccTest');
      print('Framework: Flutter + Dart');
      print('Platform: ${info.platform}');
      print('Backend: ${info.renderingBackend}');
      print('Hardware Accelerated: ${info.isHardwareAccelerated ? "Yes" : "No"}');
      print('\n${HardwareAccelerationService.getDetailedReport(info)}');
      print('=== End of Detailed Log ===\n');
      
      setState(() {
        _hardwareInfo = info;
        _isLoading = false;
      });
    } catch (e) {
      setState(() {
        _errorMessage = 'Error loading hardware info: $e';
        _isLoading = false;
      });
    }
  }

  void _refreshInfo() {
    HardwareAccelerationService.clearCache();
    _loadHardwareInfo();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: const Text('Hardware Acceleration Test'),
        actions: [
          IconButton(
            onPressed: _refreshInfo,
            icon: const Icon(Icons.refresh),
            tooltip: 'Refresh',
          ),
        ],
      ),
      body: _buildBody(),
    );
  }

  Widget _buildBody() {
    if (_isLoading) {
      return const Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            CircularProgressIndicator(),
            SizedBox(height: 16),
            Text('Loading hardware information...'),
          ],
        ),
      );
    }

    if (_errorMessage != null) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                Icons.error_outline,
                size: 64,
                color: Theme.of(context).colorScheme.error,
              ),
              const SizedBox(height: 16),
              Text(
                _errorMessage!,
                style: TextStyle(
                  color: Theme.of(context).colorScheme.error,
                  fontSize: 16,
                ),
                textAlign: TextAlign.center,
              ),
              const SizedBox(height: 16),
              ElevatedButton(
                onPressed: _refreshInfo,
                child: const Text('Retry'),
              ),
            ],
          ),
        ),
      );
    }

    if (_hardwareInfo == null) {
      return const Center(
        child: Text('No hardware information available'),
      );
    }

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          _buildOverviewCard(),
          const SizedBox(height: 16),
          _buildDetailsCard(),
          const SizedBox(height: 16),
          _buildFullReportCard(),
        ],
      ),
    );
  }

  Widget _buildOverviewCard() {
    final info = _hardwareInfo!;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(
                  info.isHardwareAccelerated 
                    ? Icons.hardware 
                    : Icons.warning,
                  color: info.isHardwareAccelerated 
                    ? Colors.green 
                    : Colors.orange,
                ),
                const SizedBox(width: 8),
                Text(
                  'Hardware Acceleration Status',
                  style: Theme.of(context).textTheme.titleLarge,
                ),
              ],
            ),
            const SizedBox(height: 16),
            _buildInfoRow(
              'Status',
              info.isHardwareAccelerated ? '✅ Enabled' : '❌ Disabled',
              valueColor: info.isHardwareAccelerated ? Colors.green : Colors.red,
            ),
            _buildInfoRow('Platform', info.platform),
            _buildInfoRow('Backend', info.renderingBackend),
            _buildInfoRow('Graphics Card', info.graphicsCard),
          ],
        ),
      ),
    );
  }

  Widget _buildDetailsCard() {
    final info = _hardwareInfo!;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.info_outline, color: Colors.blue),
                const SizedBox(width: 8),
                Text(
                  'OpenGL Information',
                  style: Theme.of(context).textTheme.titleLarge,
                ),
              ],
            ),
            const SizedBox(height: 16),
            _buildInfoRow('Version', info.openGLVersion),
            _buildInfoRow('Vendor', info.openGLVendor),
            _buildInfoRow('Renderer', info.openGLRenderer),
            if (info.surfaceFormat != 'Unknown')
              _buildInfoRow('Surface Format', info.surfaceFormat),
            _buildInfoRow(
              'Software Rendering',
              info.isSoftwareRendering ? '⚠️ Yes' : '✅ No',
              valueColor: info.isSoftwareRendering ? Colors.orange : Colors.green,
            ),
            if (info.platform.toLowerCase().contains('linux'))
              _buildInfoRow(
                'EGL Support',
                info.isEglSupported ? '✅ Available' : '❌ Not detected',
                valueColor: info.isEglSupported ? Colors.green : Colors.orange,
              ),
          ],
        ),
      ),
    );
  }

  Widget _buildFullReportCard() {
    final info = _hardwareInfo!;
    final report = HardwareAccelerationService.getDetailedReport(info);
    
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.description, color: Colors.purple),
                const SizedBox(width: 8),
                Text(
                  'Detailed Report',
                  style: Theme.of(context).textTheme.titleLarge,
                ),
              ],
            ),
            const SizedBox(height: 16),
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Theme.of(context).colorScheme.surfaceVariant,
                borderRadius: BorderRadius.circular(8),
              ),
              child: Text(
                report,
                style: const TextStyle(
                  fontFamily: 'monospace',
                  fontSize: 12,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildInfoRow(String label, String value, {Color? valueColor}) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4.0),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 120,
            child: Text(
              '$label:',
              style: const TextStyle(fontWeight: FontWeight.w500),
            ),
          ),
          Expanded(
            child: Text(
              value,
              style: TextStyle(
                color: valueColor ?? Theme.of(context).textTheme.bodyLarge?.color,
              ),
            ),
          ),
        ],
      ),
    );
  }
}
