## IRC gateway to Ricochet v3

`ricochet-irc-v3` is an IRC gateway for the Ricochet network. It enables you to use Ricochet with an IRC client.

Work in progress. Use at your own risk.

Example session using [WeeChat](https://weechat.org/):

![ricochet-irc screenshot](doc/irc/ricochet-irc.png)

### Building on Debian 10 (Buster)

#### Dependencies (incomplete)
```
apt-get install qt5-default qtbase5-dev qtbase5-dev-tools qttools5-dev-tools qtdeclarative5-dev protobuf-compiler 
apt-get install libssl-dev
```

#### Building
```
git clone -b irc-v3-2020-alpha https://github.com/wfr/ricochet-irc ricochet-irc-v3
cd ricochet-irc-v3
git submodule update --init --recursive
mkdir build
cd build
qmake PROTOBUFDIR=/usr/include/google/ ../src/
make -j$(nproc)
```

#### Running
```
cd release/irc/
./ricochet-irc
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
    @ricochet |  ___ _            _        _     ___ ___  ___       ____
    @ricochet | | _ (_)__ ___  __| |_  ___| |_  |_ _| _ \/ __| __ _|__ /
    @ricochet | |   / / _/ _ \/ _| ' \/ -_)  _|  | ||   / (__  \ V /|_ \
    @ricochet | |_|_\_\__\___/\__|_||_\___|\__| |___|_|_\\___|  \_/|___/ 1.1.4
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
           -- | ricochet has changed topic for #ricochet to "ricochet:fdmls67o6sf7ok726y5xfrxurhknoul5vg5augtes43w2eue6eu3nbad"
    @ricochet | Tor status: ready
          --> | afriend (~afriend@ricochet:sczutjtt4vobmm2fpc5w5usz5pogrliggdzwmqhgoslvo7zph764sdqd) has joined #ricochet
```

Ricochet-IRC connects to the network only when an IRC client is attached. As
soon as you close your IRC client, you will appear offline to your contacts.

### Changes
2020-11-22:
 * Rebased on Ricochet Refresh, v3-2020-alpha branch.
 * Restored compatibility with GCC 8.
 * Added v3 onion support. Removed v2 onion support.
 * TODO: clean up, remove GUI dependencies.

2017-01-15:

 * small UX improvements (input sanitation and validation)
 * prevent nickname conflicts

2017-01-13:

 * updated to v1.1.4


### Implementation notes
The embedded IRC server is hardcoded to listen on 127.0.0.1 only.

### Miscellaneous notes
#### Convert HS key to Ricochet format
```
echo $(cut -c 33-95 hs_ed25519_secret_key | base64 -w 0)
```

### License
GPLv3

### Other
Bugs can be reported on the [issue tracker](https://github.com/wfr/ricochet-irc/issues).

`Wolfgang Frisch <wfr@roembden.net>`.
