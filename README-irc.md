## IRC gateway to Ricochet

`ricochet-irc` is an IRC proxy for Ricochet. It enables you to use the Ricochet network with your favorite IRC client.

This program is work in progress. Use at your own risk!

Example session using [WeeChat](https://weechat.org/):

![ricochet-irc screenshot](doc/irc/ricochet-irc.png)

### Debug build instruction

```
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
    @ricochet | |_|_\_\__\___/\__|_||_\___|\__| |___|_|_\\___|
    @ricochet |
    @ricochet | COMMANDS:
    @ricochet |  * help
    @ricochet |  * id                      -- print your ricochet id
    @ricochet |  * add ID NICK MESSAGE     -- add a contact
    @ricochet |  * delete NICK             -- delete a contact
    @ricochet |  * rename NICK NEW_NICK    -- rename a contact
    @ricochet |  * request list            -- list incoming requests
    @ricochet |  * request accept ID NICK  -- accept incoming request
    @ricochet |  * request reject ID       -- reject incoming request
```


### Recent changes
 * Limit IRC client connections to 1.

 * Multi-line messages are supported.

 * Tor only runs when an IRC user is connected.

 * The Control channel topic shows your own ID.

 * Show online/offline status with flag: +v/-v.
 
 * Command-line parsing. Try --help.


### To do
 * i18n

 * Test with clients other than WeeChat.

 * Make Tor configurable.


### Implementation notes

The embedded IRC server is only rudimentary. It is hardcoded to listen on localhost only.

### License
GPLv3

### Other
Bugs can be reported on the [issue tracker](https://github.com/wfr/ricochet-irc/issues).

`Wolfgang Frisch <wfr@roembden.net>`.
