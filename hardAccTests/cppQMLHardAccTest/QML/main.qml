import QtQuick
import QtQuick.Window

Window {
    id: mainWindow
    visible: true
    visibility: "Windowed"
    width: 800
    height: 600
    title: qsTr("Hardware Acceleration Test - Qt/QML with Torizon")

    OpacityAnimator
    {
        id: animator
        target: mainWindow.contentItem
        from: 0
        to: 1
        duration: 1000
        running: true
    }

    HardwareAccelerationScene {
        id: main
        anchors.fill: parent
    }
}
