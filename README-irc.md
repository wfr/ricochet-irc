### IRC gateway to Ricochet

`ricochet-irc` is an IRC proxy for Ricochet. It enables you to use the Ricochet network with your favorite IRC client.

This program is work in progress. Use at your own risk!

It has only been tested with [WeeChat](https://weechat.org/).


### Build 

```
	qmake ricochet-irc.pro
	make -j$(nproc)
```

### Debug build

```
	qmake ricochet-irc.pro CONFIG+=debug
	make clean
	make -j$(nproc)
```

### Usage
```
	./ricochet-irc --help

	./ricochet-irc

	# The program now listens for an IRC connection on localhost:6667.
	# If you're running it for the first time, it generates
	# a random server password and prints it on stdout.
	
	# Point your IRC client to localhost:6667
	# => You are automatically joined into the #ricochet channel.
```

### Recent changes
 * Tor only runs when an IRC user is connected.

 * The Control channel topic shows your own ID.

 * Offline users are now shown immediately.

 * Show online/offline status with flag: +v/-v.
 
 * Command-line parsing. Try --help.

 * Implemented `rename` and `delete`

 * Send AWAY message in queries with disconnected users.
 

### To do

 * i18n

 * Test with clients other than WeeChat.


### Implementation notes

The embedded IRC server is rudimentary. It is hardcoded to listen on localhost only.
