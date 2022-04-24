## IRC gateway to Ricochet v3 (Refresh v3.0.11)

`ricochet-irc` is an IRC gateway to the Ricochet v3 network.

It is currently based on [R2: Ricochet Refresh](https://github.com/blueprint-freespeech/ricochet-refresh/),
specifically its experimental Tego-Core branch that supports v3 onions.

Please refer to [README-upstream.md](README-upstream.md) for a more detailed explanation of Ricochet itself.

Example session with [WeeChat](https://weechat.org/) as a client:

![ricochet-irc screenshot](doc/irc/ricochet-irc.png)

### Building on Debian 10 (Buster)

#### Dependencies
```
apt-get install qt5-default qtbase5-dev qtbase5-dev-tools qttools5-dev-tools qtdeclarative5-dev
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
2022-04-24:
 * Merged ricochet-refresh v3.0.11.
 * Migrated to CMake

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


### Miscellaneous
#### Convert Hidden Service key to Ricochet format
```
echo $(cut -c 33-95 hs_ed25519_secret_key | base64 -w 0)
```

### License
GPLv3

### Other
Bugs can be reported on the [issue tracker](https://github.com/wfr/ricochet-irc/issues).


### Cash for coffee
If you feel like inviting me (`Wolfgang Frisch <wfrisch@riseup.net>`) to a coffee...

* [Algorand](https://www.algorand.com): `D3CI5EKC7IRLJMGV74NSKJD6CDXDGS6DT2OQZ4T6NUAB5RIZMTEM37DODI`
* [Monero/XMR](https://www.getmonero.org/): `4245uqJmyoWbWx7w41TL2scKrgk6MtdiYR8ffFiCenJhDx227AbqbU7enMUjwYkHv9W7K6TKZDCLTEnV8ApXih3uNP1B6B3`
