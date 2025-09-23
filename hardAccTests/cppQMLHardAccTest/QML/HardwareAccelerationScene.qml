import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: root
    anchors.fill: parent
    anchors.margins: 20

    ColumnLayout {
        width: root.width - 40
        spacing: 20

        // Header
        Text {
            text: "Welcome to Qt/QML Hardware Acceleration Test with Torizon!"
            font.pixelSize: 24
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.bottomMargin: 20
            wrapMode: Text.WordWrap
        }

        // Hardware Acceleration Status
        Rectangle {
            Layout.fillWidth: true
            height: statusLayout.height + 30
            color: hardwareService.isHardwareAccelerated ? "#4CAF50" : "#F44336"
            radius: 8

            ColumnLayout {
                id: statusLayout
                anchors.centerIn: parent
                spacing: 10

                Text {
                    text: "Hardware Acceleration Status"
                    font.pixelSize: 18
                    font.bold: true
                    color: "white"
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: hardwareService.isHardwareAccelerated ? 
                          "✅ Hardware Acceleration: ENABLED" : 
                          "❌ Hardware Acceleration: DISABLED (Software Rendering)"
                    font.pixelSize: 16
                    font.bold: true
                    color: "white"
                    Layout.alignment: Qt.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }
        }

        // System Information
        Rectangle {
            Layout.fillWidth: true
            height: infoLayout.height + 30
            color: "#E0E0E0"
            radius: 8

            ColumnLayout {
                id: infoLayout
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 15
                spacing: 10

                Text {
                    text: "System Information"
                    font.pixelSize: 18
                    font.bold: true
                    color: "black"
                }

                Text {
                    text: hardwareService.isLoading ? "Loading hardware information..." : hardwareService.hardwareInfo
                    font.family: "monospace"
                    font.pixelSize: 12
                    color: "black"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // Detailed Report (Expandable)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                Layout.fillWidth: true
                height: 50
                color: "#2196F3"
                radius: 4

                MouseArea {
                    anchors.fill: parent
                    onClicked: detailedReport.visible = !detailedReport.visible
                }

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 10

                    Text {
                        text: "Detailed Hardware Report"
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                    }

                    Text {
                        text: detailedReport.visible ? "▼" : "▶"
                        font.pixelSize: 16
                        color: "white"
                    }
                }
            }

            Rectangle {
                id: detailedReport
                Layout.fillWidth: true
                height: visible ? reportText.height + 20 : 0
                color: "#F5F5F5"
                radius: 4
                visible: false

                Behavior on height {
                    NumberAnimation { duration: 200 }
                }

                Text {
                    id: reportText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                    text: hardwareService.detailedReport
                    font.family: "monospace"
                    font.pixelSize: 11
                    color: "black"
                    wrapMode: Text.WordWrap
                }
            }
        }

        // Instructions
        Rectangle {
            Layout.fillWidth: true
            height: instructionsLayout.height + 30
            color: "#E3F2FD"
            radius: 8

            ColumnLayout {
                id: instructionsLayout
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 15
                spacing: 10

                Text {
                    text: "Instructions"
                    font.pixelSize: 16
                    font.bold: true
                    color: "black"
                }

                Text {
                    text: "This application tests hardware acceleration capabilities on your system. " +
                          "Green status indicates hardware acceleration is working. " +
                          "Red status indicates software rendering is being used.\n\n" +
                          "For optimal performance on embedded systems, ensure:\n" +
                          "• GPU drivers are properly installed\n" +
                          "• EGL libraries are available (Linux)\n" +
                          "• Hardware acceleration is enabled in system settings"
                    font.pixelSize: 12
                    color: "black"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // Refresh Button
        Button {
            text: "Refresh Hardware Information"
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10
            onClicked: hardwareService.refreshHardwareInfo()
            enabled: !hardwareService.isLoading
        }

        // Loading indicator
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            visible: hardwareService.isLoading
            running: hardwareService.isLoading
        }
    }
}