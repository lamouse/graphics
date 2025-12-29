pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import RenderCtrl

ApplicationWindow {
    id: mainWindow
    width: 640
    height: 480
    visible: true
    title: qsTr("graphics config")

    // 顶部菜单栏
    menuBar: MenuBar {
        Menu {
            title: qsTr("文件")
            MenuItem {
                text: qsTr("选择文件")
                onTriggered: fileDialog.open()
            }
            MenuItem {
                text: qsTr("启动游戏")
                onTriggered: {
                    stackView.push(gameView);
                }
            }
            MenuItem {
                text: qsTr("返回游戏列表")
                onTriggered: {
                    stackView.pop();
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "选择文件"
        currentFolder: "."

        nameFilters: ["所有文件(.*)"]
        onAccepted: {
            console.log("选择了文件：", fileDialog.selectedFile);
        }
        onRejected: console.log("取消了选择")
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: gameList
    }

    Component {
        id: gameList
        RenderControl{
            onRenderStart:{
                console.log("render start")
                mainWindow.close()
                Qt.quit()
            }
        }
    }

    Component {
        id: gameView
        Rectangle {
            color: "black"
            Text {
                anchors.centerIn: parent
                color: "white"
                text: "正在运行的游戏..."
            }
        }
    }
}
