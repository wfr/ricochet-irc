### IRC frontend for Ricochet

`ricochet-irc` is a headless IRC frontend for Ricochet. It enables you to use the Ricochet network with your favorite IRC client.

*Work in progress!*


### Build 

```
	qmake ricochet-irc.pro
	make -j$(nproc)
```

### Debug build

```
	qmake ricochet-irc.pro CONFIG+=debug CONFIG+=no-hardened
	make clean
	make -j$(nproc)
```

### Usage
```
	./ricochet-irc --help
	./ricochet-irc --port 12345
	
	# connect to localhost/12345
	# you are automatically joined into #ricochet
	# type: help
```

### Recent changes
 * Tor only runs when an IRC user is connected.

 * The Control channel topic shows your own ID.

 * Offline users are now shown immediately.

 * Show online/offline status with flag: +v/-v.

 * Fixed various IRC bugs.
 
 * Command-line parsing. Try --help.

 * Implemented `rename` and `delete`

 * Send AWAY message in query with disconnected user.
 
 * IRC user is automatically joined into #ricochet.


### To do

 * Deal with rejected outgoing requests

 * Support IRC password protection.

 * i18n

 * Provide instructions on disabling logging for various IRC clients 


### Implementation notes

The embedded IRC server is minimalistic and not intended to implement
the complete RFC. It is hardcoded to listen on localhost only.
