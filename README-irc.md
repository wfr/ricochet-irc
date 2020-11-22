## IRC gateway to Ricochet

`ricochet-irc` is an IRC gateway for the Ricochet network. It enables you to use Ricochet with an IRC client.

Work in progress. Use at your own risk.

Example session using [WeeChat](https://weechat.org/):

![ricochet-irc screenshot](doc/irc/ricochet-irc.png)

### Installation

```
	git clone https://github.com/wfr/ricochet-irc
	cd ricochet-irc
	git checkout irc

	qmake ricochet-irc.pro CONFIG+=debug CONFIG+=no-hardened
	make clean
	make -j$(nproc)
```

### Usage
```
    ./ricochet-irc
```
The program is now waiting for an IRC connection on localhost:6667.
Point your IRC client to localhost:6667 and use the password that is provided on stdout.

For custom options, try:
```
    ./ricochet-irc --help
```

#### IRC interface
Once you are connected to the IRC server, your client is automatically joined into the `#ricochet` control channel.

```
    @ricochet |  ___ _            _        _     ___ ___  ___
    @ricochet | | _ (_)__ ___  __| |_  ___| |_  |_ _| _ \/ __|
    @ricochet | |   / / _/ _ \/ _| ' \/ -_)  _|  | ||   / (__
    @ricochet | |_|_\_\__\___/\__|_||_\___|\__| |___|_|_\\___| 1.1.4
    @ricochet |
    @ricochet | COMMANDS:
    @ricochet |  * help
    @ricochet |  * id                            -- print your ricochet id
    @ricochet |  * add ID NICKNAME MESSAGE       -- add a contact
    @ricochet |  * delete NICKNAME               -- delete a contact
    @ricochet |  * rename NICKNAME NEW_NICKNAME  -- rename a contact
    @ricochet |  * request list                  -- list incoming requests
    @ricochet |  * request accept ID NICKNAME    -- accept incoming request
    @ricochet |  * request reject ID             -- reject incoming request
    @ricochet |
    @ricochet | Tor status: offline
           -- | ricochet has changed topic for #ricochet to "ricochet:neohosiepuos2txr"
    @ricochet | Tor status: ready
          --> | afriend (~afriend@ricochet:daag4pbdutthcpun) has joined #ricochet
```

Ricochet-IRC connects to the network only when an IRC client is attached. As
soon as you close your IRC client, you will appear offline to your contacts.

### Changes
2017-01-15:

 * small UX improvements (input sanitation and validation)
 * prevent nickname conflicts

2017-01-13:

 * updated to v1.1.4


### Implementation notes

The embedded IRC server is hardcoded to listen on 127.0.0.1 only.

### License
GPLv3

### Other
Bugs can be reported on the [issue tracker](https://github.com/wfr/ricochet-irc/issues).

`Wolfgang Frisch <wfr@roembden.net>`.
