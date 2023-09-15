pragma Singleton

import QtQuick

/**
* Global styles. This is just an example how to define styles. These styles can
* be used in QML-files like this:
* <pre>
* ...
* import Style
* ...
* Label {
*   color: Style.text.color.primary
*   text: qsTr("example")
* }
* ...
* </pre>
*/
QtObject {
    id: base

    property QtObject color: QtObject {
        readonly property color primary:  Qt.rgba(0, 220, 0, 1)
        readonly property color secondary:  Qt.rgba(100, 100, 100, 1)

        readonly property color background:  Qt.rgba(255, 255, 255, 1)
        readonly property color surface:  Qt.rgba(100, 100, 100, 1)
        readonly property color error:  Qt.rgba(255, 0, 0, 1)
    }

    property QtObject header: QtObject {
        property QtObject color: QtObject {
            readonly property color primary:  base.color.primary
        }
    }

    property QtObject text: QtObject {
        property QtObject color: QtObject {
            readonly property color primary:  base.color.primary
            readonly property color secondary: Qt.rgba(200, 0, 0, 1)
        }
    }

    property QtObject background: QtObject {
        property QtObject color: QtObject {
            readonly property color primary:  base.color.background
            readonly property color secondary: base.color.surface
        }
    }

    property QtObject table: QtObject {
        property QtObject color: QtObject {
            readonly property color primary:  base.color.background
            readonly property color selected:  base.color.error
        }
    }

    property color colorWhite: "white"
    property color colorWarning: "yellow"
    property color colorSuccess: "green"
    property color colorAccent: "blue"
    property color colorVariant: "darkviolet"
    property color colorFailure: "darkred"
    property color colorError: "red"
    property string primaryFont: "Segoe UI"

}
