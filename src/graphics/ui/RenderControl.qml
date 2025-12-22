import QtQuick
import QtQuick.Controls

Window {
    id: renderCtrl
    width: 800
    height: 600
    signal renderStart

    visible: true
    Rectangle {
        color: "lightgray"
        anchors.fill: parent
        focus: true
        Keys.onPressed: {
            renderCtrl.renderStart();
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

            Switch {
                id: debug_ui
                text: "开启调试UI"
                checked: render_ctrl.isUseDebugUIEnabled()
                onCheckedChanged: render_ctrl.setUseDebugUI(checked)
            }

            Button {
                id: began_render
                text: "开启渲染"
                onClicked: {
                    renderCtrl.renderStart();
                }
            }
        }
    }
}
