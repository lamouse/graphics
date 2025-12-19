import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

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
        Rectangle {
            color: "lightgray"

            // 居中的文本
            Text {
                anchors.centerIn: parent
                text: "这里显示游戏列表"
            }

            // 布局容器：垂直排列 CheckBox 和 Button
            Column {
                spacing: 12  // 控件间距（可调）
                anchors {
                    horizontalCenter: parent.horizontalCenter  // 水平居中
                    top: parent.top
                    topMargin: 60  // 距离顶部留空，避免和 Text 重叠
                }

                Switch {
                    id: render_debug
                    text: "开启渲染调试"
                    checked: render_ctrl.isRenderDebugEnabled()
                    onCheckedChanged: render_ctrl.handleOptionRenderDebug(checked)
                }

                Button {
                    id: began_render
                    text: "开启渲染"
                    onClicked: {
                        console.log("渲染已开启！");
                        // render_ctrl.startRendering(); // 如果有这个方法
                    }
                }
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
