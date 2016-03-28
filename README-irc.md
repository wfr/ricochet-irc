### IRC frontend for Ricochet

`ricochet-irc` is a headless IRC frontend for Ricochet. It enables you to use the Ricochet network with your favorite IRC client.

*Work in progress!*


### Build instructions

```
	qmake ricochet-irc.pro
	make -j$(nproc)
```

### Usage
```
	./ricochet-irc
	
	# connect to localhost/6667
	# you are automatically joined into #ricochet
	# type: help
```

### Recent changes

 * rename works (no strict checking, beware of duplicate names)

 * added standalone IRC demo `irc-server-example`
 
 * implemented NICK/JOIN/PART/QUIT

 * --port 12345

 * send AWAY message in query with disconnected user
 
 * show online/offline status with flag: +v/-v

 * autojoin #ricochet
 
 * the executable name was changed to `ricochet-irc`


### Plan

 * add support for IRC server passwords

 * replace user-facing QStringLiterals with i18n


### Implementation notes

The Qt based IRC server is very minimalistic and does not intend to implement
the full RFC. It is NOT intended for public connections. It is hardcoded to
listen on localhost only.
