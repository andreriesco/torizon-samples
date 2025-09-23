# Flutter Template Specific Documentation

> ⚠️ **WARNING:**  This is just the documentation part specific of this template. **For the complete and general Torizon IDE documentation, check the [developer website documentation](https://developer.toradex.com/torizon/application-development/ide-extension/)** ⚠️

All projects follow the pipeline of tasks described in the [common contributing documentation](https://github.com/toradex/vscode-torizon-templates/blob/bookworm/CONTRIBUTING.md#contributing-templates). However, each project has its own specificities in terms of technologies and methods used to compile, deploy, and debug the code. Therefore, each of them has their own specific tasks in the **tasks.json** file.

This Flutter template uses the Dart compiler to compile the code,with tasks named **flutter-build-debug-\${architecture}** (compiles the code). It also uses an SDK container to cross-compile the code. This container image is built using the **Dockerfile.sdk** file, and the tasks that build the containers are named **build-container-image-sdk-\${architecture}**.

The compiled code is then copied into the running debug container using **scp**, in the task named **deploy-torizon-\${architecture}**. This task contains the entire sequence of tasks executed by the pipeline and, therefore, is unique to each template.

Finally, remote debugging is performed by attaching to the GDB on the running container on the device using a [VSCode feature called Pipe Transport](https://code.visualstudio.com/docs/cpp/pipe-transport).

The source code of the template is a simple UI with Flutter and Dart.

Please check out [Flutter Documentation](https://docs.flutter.dev/get-started/fundamentals) to learn more about Flutter.

This Template is maintained by [Crossware.io](https://www.crossware.io/).

## Local Debugging And Remote Debugging

> ⚠️ **WARNING:** If an error occurs on the first time you try to run the debugger, specially after having executed the `installFlutter.sh` script, please reload the VSCode window (using for example the `reload-vscode-window` task on `TASK RUNNER`) and try it again ⚠️

The `.dart` files, inside `lib`, are common for both remote debugging and local debugging.

For remote debugging, the C++ files used are the ones inside the `elinux` folder.

⚠️ Debugging for `.dart` files is currently broken for remote debugging and a fix will be added soon. So, it's possible to debug just the C++ files for now.

For local debugging, the C++ files used are the ones inside the `linux` folder.

Debugging of `.dart` files and Flutter VSCode extension features (DevTools) are working for local debugging. So, generally it's not necessary to debug the C++ files, but if you need to do it, you can add a GDB task on `launch.json`, like this:

```
        {
            "name": "Local AMD64",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/linux/x64/debug/bundle/sample",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceRoot}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description":  "Set Disassembly Flavor to Intel",
                    "text": "-gdb-set disassembly-flavor intel",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "flutter-resolve-dependencies-local"
        },
```
