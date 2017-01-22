import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import im.ricochet 1.0

ColumnLayout {
    anchors {
        fill: parent
        margins: 8
    }

    CheckBox {
        text: qsTr("Use a single window for conversations")
        checked: uiSettings.data.combinedChatWindow || false
        onCheckedChanged: {
            uiSettings.write("combinedChatWindow", checked)
        }
    }

    CheckBox {
        text: qsTr("Open links in default browser without prompting")
        checked: uiSettings.data.alwaysOpenBrowser || false
        onCheckedChanged: {
            uiSettings.write("alwaysOpenBrowser", checked)
        }
    }

    CheckBox {
        text: qsTr("Play audio notifications")
        checked: uiSettings.data.playAudioNotification || false
        onCheckedChanged: {
            uiSettings.write("playAudioNotification", checked)
        }
    }
    RowLayout {
        Item { width: 16 }

        Label { text: qsTr("Volume") }

        Slider {
            maximumValue: 1.0
            updateValueWhileDragging: false
            enabled: uiSettings.data.playAudioNotification || false
            value: uiSettings.read("notificationVolume", 0.75)
            onValueChanged: {
                uiSettings.write("notificationVolume", value)
            }
        }
    }

    RowLayout {
        z: 2
        Label { text: qsTr("Language") }

        ComboBox {
            id: languageBox
            model: languageModel
            textRole: "nativeName"
            currentIndex: languageModel.rowForLocaleID(uiSettings.data.language)
            Layout.minimumWidth: 200

            LanguagesModel {
                id: languageModel
            }

            onActivated: {
                var localeID = languageModel.localeID(index)
                uiSettings.write("language", localeID)
                restartBubble.displayed = true
                bubbleResetTimer.start()
            }

            Bubble {
                id: restartBubble
                target: languageBox
                text: qsTr("Restart Ricochet to apply changes")
                displayed: false
                horizontalAlignment: Qt.AlignRight

                Timer {
                    id: bubbleResetTimer
                    interval: 3000
                    onTriggered: restartBubble.displayed = false
                }
            }
        }
    }

    RowLayout {
        CheckBox {
            id: deleteMessagesBox
            text: qsTr("Delete messages after")
            checked: uiSettings.data.deleteMessages || false
            onCheckedChanged: {
                uiSettings.write("deleteMessages", checked)
                deleteMessagesDurationBox.enabled = checked
            }
        }

        ComboBox {
            id: deleteMessagesDurationBox
            Layout.minimumWidth: 200
            enabled: deleteMessagesBox.checked
            model: [
                { "text": "10 " + qsTr("seconds"),  "seconds": 10 },
                { "text": "15 " + qsTr("minutes"),  "seconds": 60 * 15 },
                { "text": "30 " + qsTr("minutes"),  "seconds": 60 * 30 },
                { "text": "60 " + qsTr("minutes"),  "seconds": 60 * 60 },
                { "text": "6 " + qsTr("hours"),     "seconds": 60 * 60 * 6 },
                { "text": "12 " + qsTr("hours"),    "seconds": 60 * 60 * 12 },
                { "text": "24 " + qsTr("hours"),    "seconds": 60 * 60 * 24 },
                { "text": "7 " + qsTr("days"),      "seconds": 60 * 60 * 24 * 7 },
                { "text": "14 " + qsTr("days"),     "seconds": 60 * 60 * 24 * 14 },
                { "text": "30 " + qsTr("days"),     "seconds": 60 * 60 * 24 * 30  },
            ]
            textRole: "text"

            function findBySeconds(seconds) {
                var closest_index;
                var closest_diff = 1<<30;
                for (var i = 0; i < model.length; i++) {
                    var diff = Math.abs(model[i].seconds - seconds);
                    if (diff < closest_diff) {
                        closest_diff = diff;
                        closest_index = i;
                    }
                }
                return closest_index;
            }

            currentIndex: findBySeconds(uiSettings.data.deleteMessagesAfter || 1<<30)
            onCurrentIndexChanged: {
                uiSettings.write("deleteMessagesAfter", model[currentIndex].seconds);
            }
        }
    }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}
