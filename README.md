# ricochet-irc
## IRC interface to Ricochet-Refresh (v3.0.11)
`ricochet-irc` is an IRC interface to the
[R2: Ricochet-Refresh](https://github.com/blueprint-freespeech/ricochet-refresh/)
P2P chat network.

With this, you can use your favorite IRC client as a UI for Ricochet, for
example [WeeChat](https://weechat.org/):

![ricochet-irc screenshot](doc/irc/ricochet-irc.png)

For an introduction to Ricochet itself, please refer to
[README-upstream.md](README-upstream.md).

### Building on Debian 11 (Bullseye)

#### Dependencies
```
apt-get install build-essential cmake
apt-get install qtbase5-dev qtbase5-dev-tools qttools5-dev-tools qttools5-dev qtdeclarative5-dev
apt-get install protobuf-compiler libssl-dev
```

#### Building
```
git clone -b irc https://github.com/wfr/ricochet-irc ricochet-irc
cd ricochet-irc
git submodule update --init --recursive
mkdir build
cd build
cmake ../src/
make -j$(nproc)
```

### Usage
```
    ./ricochet-irc
```
The program is now waiting for an IRC connection on localhost:6667.  Point your
IRC client to localhost:6667 and use the password that is provided on stdout.

For custom options, try:
```
    ./ricochet-irc --help
```

#### IRC interface
Once you are connected to the IRC server, your client is automatically joined
into a control channel `#ricochet`:

```
    @ricochet |  ___ _            _        _     ___ ___  ___       ____
    @ricochet | | _ (_)__ ___  __| |_  ___| |_  |_ _| _ \/ __| __ _|__ /
    @ricochet | |   / / _/ _ \/ _| ' \/ -_)  _|  | ||   / (__  \ V /|_ \
    @ricochet | |_|_\_\__\___/\__|_||_\___|\__| |___|_|_\\___|  \_/|___/ devbuild
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

##### Online/Offline status
`ricochet-irc` connects to the Ricochet network when an IRC client is
attached. You will appear offline to your contacts as soon as you disconnect
from the IRC server.

### Changes
[CHANGES.md](CHANGES.md)

### Miscellaneous notes
#### Convert Hidden Service key to Ricochet format
```
echo $(cut -c 33-95 hs_ed25519_secret_key | base64 -w 0)
```

### License
GPLv3

### Other
Please report bugs in the [issue tracker](https://github.com/wfr/ricochet-irc/issues).


### Cash for coffee
If you feel like inviting me (`Wolfgang Frisch <wfrisch@riseup.net>`) to a coffee...

* [Algorand](https://www.algorand.com): `D3CI5EKC7IRLJMGV74NSKJD6CDXDGS6DT2OQZ4T6NUAB5RIZMTEM37DODI`
* [Monero/XMR](https://www.getmonero.org/): `4245uqJmyoWbWx7w41TL2scKrgk6MtdiYR8ffFiCenJhDx227AbqbU7enMUjwYkHv9W7K6TKZDCLTEnV8ApXih3uNP1B6B3`
