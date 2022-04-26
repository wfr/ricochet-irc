import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

Column {
    id: setup
    spacing: 8

    property alias proxyType: proxyTypeField.selectedType
    property alias proxyAddress: proxyAddressField.text
    property alias proxyPort: proxyPortField.text
    property alias proxyUsername: proxyUsernameField.text
    property alias proxyPassword: proxyPasswordField.text
    property alias allowedPorts: allowedPortsField.text
    property alias bridgeType: bridgeTypeField.selectedType
    property string selectedInbuiltBridgeStrings
    property alias customBridges: customBridgeStringField.text

    function reset() {
        proxyTypeField.currentIndex = 0
        proxyAddress = ''
        proxyPort = ''
        proxyUsername = ''
        proxyPassword = ''
        allowedPorts = ''
        customBridges = ''
        selectedInbuiltBridgeStrings = ''
    }

    function save() {
        var conf = {};
        conf.disableNetwork = "0";
        conf.proxyType = proxyType;
        conf.proxyAddress = proxyAddress;
        conf.proxyPort = proxyPort;
        conf.proxyUsername = proxyUsername;
        conf.proxyPassword = proxyPassword;
        conf.allowedPorts = allowedPorts.trim().length > 0 ? allowedPorts.trim().split(',') : [];
        var bridgeStrings = "";
        if (bridgeType == "custom") {
            bridgeStrings = customBridges;
        } else {
            bridgeStrings = selectedInbuiltBridgeStrings;
        }
        conf.bridges = bridgeStrings.trim().length > 0 ? bridgeStrings.trim().split('\n') : [];

        var command = torControl.setConfiguration(conf)
        command.finished.connect(function() {
            if (command.successful) {
                if (torControl.hasOwnership)
                    torControl.saveConfiguration()
                window.openBootstrap()
            } else
                console.log("SETCONF error:", command.errorMessage)
        })
    }

    Label {
        width: parent.width
        text: qsTr("Does this computer need a proxy to access the internet?")
        wrapMode: Text.Wrap

        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }

    GroupBox {
        width: setup.width

        GridLayout {
            anchors.fill: parent
            columns: 2

            /* without this the top of groupbox clips into the first row */
            Item { height: Qt.platform.os === "linux" ? 15 : 0}
            Item { height: Qt.platform.os === "linux" ? 15 : 0}

            Label {
                text: qsTr("Proxy type:")
                color: proxyPalette.text
            }
            ComboBox {
                id: proxyTypeField
                property string none: qsTr("None")
                model: [
                    { "text": qsTr("None"), "type": "" },
                    { "text": "SOCKS 4", "type": "socks4" },
                    { "text": "SOCKS 5", "type": "socks5" },
                    { "text": "HTTPS", "type": "https" },
                ]
                textRole: "text"
                property string selectedType: currentIndex >= 0 ? model[currentIndex].type : ""

                SystemPalette {
                    id: proxyPalette
                    colorGroup: setup.proxyType == "" ? SystemPalette.Disabled : SystemPalette.Active
                }

                Accessible.role: Accessible.ComboBox
                Accessible.name: selectedType
                //: Description used by accessibility tech, such as screen readers
                Accessible.description: qsTr("If you need a proxy to access the internet, select one from this list.")
            }

            Label {
                //: Label indicating the textbox to place a proxy IP or URL
                text: qsTr("Address:")
                color: proxyPalette.text

                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: proxyAddressField
                    Layout.fillWidth: true
                    enabled: setup.proxyType
                    //: Placeholder text of text box expecting an IP or URL for proxy
                    placeholderText: qsTr("IP address or hostname")

                    Accessible.role: Accessible.EditableText
                    Accessible.name: placeholderText
                    //: Description of what to enter into the IP textbox, used by accessibility tech such as screen readers
                    Accessible.description: qsTr("Enter the IP address or hostname of the proxy you wish to connect to")
                }
                Label {
                    //: Label indicating the textbox to place a proxy port
                    text: qsTr("Port:")
                    color: proxyPalette.text

                }
                TextField {
                    id: proxyPortField
                    Layout.preferredWidth: 50
                    enabled: setup.proxyType

                    Accessible.role: Accessible.EditableText
                    //: Name of the port label, used by accessibility tech such as screen readers
                    Accessible.name: qsTr("Port")
                    //: Description of what to enter into the Port textbox, used by accessibility tech such as screen readers
                    Accessible.description: qsTr("Enter the port of the proxy you wish to connect to")
                }
            }

            Label {
                //: Label indicating the textbox to place the proxy username
                text: qsTr("Username:")
                color: proxyPalette.text

                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
            RowLayout {
                Layout.fillWidth: true

                TextField {
                    id: proxyUsernameField
                    Layout.fillWidth: true
                    enabled: setup.proxyType
                    //: Textbox placeholder text indicating the field is not required
                    placeholderText: qsTr("Optional")

                    Accessible.role: Accessible.EditableText
                    //: Name of the username label, used by accessibility tech such as screen readers
                    Accessible.name: qsTr("Username")
                    //: Description to enter into the Username textbox, used by accessibility tech such as screen readers
                    Accessible.description: qsTr("If required, enter the username for the proxy you wish to connect to")
                }
                Label {
                    //: Label indicating the textbox to place the proxy password
                    text: qsTr("Password:")
                    color: proxyPalette.text

                    Accessible.role: Accessible.StaticText
                    Accessible.name: text
                }
                TextField {
                    id: proxyPasswordField
                    Layout.fillWidth: true
                    enabled: setup.proxyType
                    //: Textbox placeholder text indicating the field is not required
                    placeholderText: qsTr("Optional")

                    Accessible.role: Accessible.EditableText
                    //: Name of the password label, used by accessibility tech such as screen readers
                    Accessible.name: qsTr("Password")
                    //: Description to enter into the Password textbox, used by accessibility tech such as screen readers
                    Accessible.description: qsTr("If required, enter the password for the proxy you wish to connect to")
                }
            }
        }
    }

    Item { height: 4; width: 1 }

    Label {
        width: parent.width
        //: Description for the purpose of the Allowed Ports textbox
        text: qsTr("Does this computer's Internet connection go through a firewall that only allows connections to certain ports?")
        wrapMode: Text.Wrap

        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }

    GroupBox {
        width: parent.width
        // Workaround OS X visual bug
        height: Math.max(implicitHeight, 40)

        /* without this the top of groupbox clips into the first row */
        ColumnLayout {
            anchors.fill: parent

            Item { height: Qt.platform.os === "linux" ? 15 : 0 }

            RowLayout {
                Label {
                    //: Label indicating the textbox to place the allowed ports
                    text: qsTr("Allowed ports:")

                    Accessible.role: Accessible.StaticText
                    Accessible.name: text
                }
                TextField {
                    id: allowedPortsField
                    Layout.fillWidth: true
                    //: Textbox showing an example entry for the firewall allowed ports entry
                    placeholderText: qsTr("Example: 80,443")

                    Accessible.role: Accessible.EditableText
                    //: Name of the allowed ports label, used by accessibility tech such as screen readers
                    Accessible.name: qsTr("Allowed ports") // todo: translations
                    Accessible.description: placeholderText
                }
            }
        }
    }

    Item { height: 4; width: 1 }

    Label {
        width: parent.width

        text: qsTr("If this computer's Internet connection is censored, you will need to obtain and use bridge relays.")
        wrapMode: Text.Wrap

        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }

    GroupBox {
        width: parent.width

        GridLayout {
            columns: 2 

            // Stuffing the row layout into a column layout and inserting a bogus
            // item prevents clipping on linux
            Item { height: Qt.platform.os === "linux" ? 15 : 0 }
            Item { height: Qt.platform.os === "linux" ? 15 : 0 }

            RowLayout {
                width: parent.width

                Label {
                    text: qsTr("Bridge type:")
                }

                ComboBox {
                    // Displays the selection of a bridge type (obfs4, meek-azure, etc)
                    id: bridgeTypeField
                    property string none: qsTr("None")
                    model: ListModel {
                        id: bridgeTypeModel
                        ListElement { text: qsTr("None"); type: "none" }
                        ListElement { text: qsTr("Custom"); type: "custom" }
                        Component.onCompleted: {
                            var bridgeTypes = torControl.getBridgeTypes();
                            for (var i = 0; i < bridgeTypes.length; i++)
                            {
                                // Dynamically construct the list model so that whenever
                                // new bridge types are introduced, they'll automatically
                                // propogate to the dropdown
                                bridgeTypeModel.append({text: bridgeTypes[i], type: bridgeTypes[i]});
                            }
                        }
                    }
                    textRole: "text"

                    property string selectedType: currentIndex >= 0 ? model.get(currentIndex).type : ""

                    onCurrentIndexChanged: {
                        setup.selectedInbuiltBridgeStrings = "";

                        var bridgeStrings = torControl.getBridgeStringsForType(bridgeTypeModel.get(currentIndex).type);
                        // first, clear the bridge string list
                        for (var i = bridgeStringModel.count; i > 0; i--)
                        {
                            bridgeStringModel.remove(i - 1);
                        }

                        // now, dynamically set the new list
                        for (var i = 0; i < bridgeStrings.length; i++)
                        {
                            // Dynamically construct the list model so that whenever
                            // new bridge strings are introduced, they'll automatically
                            // propogate to the dropdown

                            // First, create a "pretty" string for the bridge
                            // This is just the bridge string with the cert and
                            // other not so nice things to read removed
                            //TODO: wherever the UI that displays this is, it'd be nice to have an onHover-type event to display the full bridge string
                            // displaying the full bridge string is rather clunky and usually overflows the width of the window anyway, and elliding it
                            // at the end encounters the same issue as creating a shortened pretty string - i.e. no way to manually check the fingerprint
                            // or cert of a bridge
                            var index = bridgeStrings[i].indexOf(' ', bridgeStrings[i].indexOf(' ') + 1);
                            var prettyString = bridgeStrings[i].substr(0, index);
                            bridgeStringModel.append({"title": prettyString, "bridgeString": bridgeStrings[i]});

                            // Then, update the custom bridge string entry, such
                            // that when the "connect" button is pressed the bridge
                            // strings are handled in the same way that custom bridge
                            // entries are handled
                            setup.selectedInbuiltBridgeStrings += bridgeStrings[i] + "\n";
                            console.log(setup.selectedInbuiltBridgeStrings)
                        }
                    }

                    SystemPalette {
                        id: bridgePalette
                        colorGroup: setup.bridgeType == "" ? SystemPalette.Disabled : SystemPalette.Active
                    }

                    Accessible.role: Accessible.ComboBox
                    Accessible.name: selectedType
                    //: Description used by accessibility tech, such as screen readers
                    Accessible.description: qsTr("If you need a bridge to access Tor, select one from this list.")
                }
            }
        }
    }

    GroupBox {
        width: parent.width

        visible: setup.bridgeType == "custom"

        ColumnLayout {
            anchors.fill: parent

            Item { height: Qt.platform.os === "linux" ? 15 : 0 }

            Label {
                text: qsTr("Enter one or more bridge relays (one per line):")

                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
            TextArea {
                id: customBridgeStringField
                Layout.fillWidth: true
                Layout.preferredHeight: allowedPortsField.height * 2
                tabChangesFocus: true

                Accessible.name: qsTr("Enter one or more bridge relays (one per line):")
                Accessible.role: Accessible.EditableText
            }
        }
    }

    //FIXME This UI is quite broken
    // it functions fine, but:
    //  1) the height is fixed, and that's not aestheticly pleasing
    //  2) the bridge strings clip through the top /shrug
    GroupBox {
        width: parent.width

        visible: setup.bridgeType != "custom" && setup.bridgeType != "none"

        ColumnLayout {
            Item { height: Qt.platform.os === "linux" ? 15 : 0 }

            Label {
                text: qsTr("Your bridges:")
                color: bridgePalette.text

                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
        }
    }

    GroupBox {
        width: parent.width

        visible: setup.bridgeType != "custom" && setup.bridgeType != "none"

        ListView {
            id: bridgeStringList
            height: 300

            model: ListModel {
                id: bridgeStringModel
                ListElement { title: "None"; bridgeString: "None" }
                ListElement { title: "None"; bridgeString: "None" }
            }

            delegate: Row {
                width: parent.width
                Text {
                    text: title
                    width: setup.width
                    elide: Text.ElideRight
                }
            }
        }
    }

    RowLayout {
        width: parent.width

        Button {
            //: Button label for going back to previous screen
            text: qsTr("Back")
            onClicked: window.back()

            Accessible.name: text
            Accessible.onPressAction: window.back()
        }

        Item { height: 1; Layout.fillWidth: true }

        Button {
            //: Button label for connecting to tor
            text: qsTr("Connect")
            isDefault: true
            enabled: setup.proxyType ? (proxyAddressField.text && proxyPortField.text) : true
            onClicked: {
                setup.save()
            }

            Accessible.name: text
            Accessible.onPressAction: setup.save()
        }
    }
}
