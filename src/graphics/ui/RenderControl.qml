import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

//import Graphics

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
        RenderController {
            id: render_ctrl
        }
        // 布局容器：垂直排列 CheckBox 和 Button
        Column {
            spacing: 12  // 控件间距（可调）
            anchors {
                horizontalCenter: parent.horizontalCenter  // 水平居中
                top: parent.top
                topMargin: 60  // 距离顶部留空，避免和 Text 重叠
            }
            RowLayout {
                id: resolutionRowId
                Label {
                    text: "分辨率："
                    Layout.alignment: Qt.AlignVCenter
                }
                ComboBox {
                    id: resolutionComboBox
                    model: ResolutionModel {}
                    onActivated: index => model.onItemSelected(index)
                }
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
