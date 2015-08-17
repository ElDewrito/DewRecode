# Changes since ED
- UPnP port forwarding for game/info server/voip, and standardized port ranges (game uses 11774 - 11783, info server uses 11784 - 11793, voip uses 11794 - 11803, users can just forward 11774-11803 and forget about it)
- Plugin support (99% of main code is exported for plugins, modules can easily be swapped to and from the main DLL into a satellite plugin DLL)
- Fixed H3 menus (menu options don't go superspeed anymore, pause menu can be closed, the main menu settings menu can be shown but probably isn't needed)
- Dual-wielding, and semi-working equipment

# Soon(tm)
- Port latest ElDorito additions (grenade fix, custom packets, VoIP disabling/muting...)
- Built-in update checker using a small update exe to perform the actual update. Updates will also be signed to prevent people from tampering with updates.
- No more launcher/file verification on start. Instead an installer will take the launchers responsibilites of verifying files and installing the latest DR files.
- Matchmaking: Master servers could create lobbies for users trying to find a game, with the masters also holding stats data a ranking system could also be implemented. Matchmaking games could have stats in a different area to "casual" stats too.
- A proper menu built-in to DR instead of an overlay will allow us to fully utilize a new menu, events can be forwarded to and from the new menu directly via binded events/callbacks, making the menu actually feel part of the game properly.
- Improved code, seperating hooks from modules and allowing code to be reused much easier.

#### Revamped stats:
Luckily the old stats system wasn't used, but the new one will track stats per-event, recording:
- what happened (player kill, player death, weapon picked up, vehicle entered...)
- where it happened (for heatmap generation)
- who it happened to

Sending this information to the master servers signed with the users keypair, to verifiably prove that event happened in that game.  
All players on the server will record this data, if Player1 kills Player2 both players would send both a player killed and player death event.  

Stat hacking will be much harder than with the older system as hackers would have to send events instead of just forging endgame info.  
And as each player sends every event that happened a correlation system could be made where each user helps to prove an event happened.  
With correlation and some rules in place (eg. stats only accepted from servers with 8+ players) stat hacking will be nigh impossible.

#### Master server syncronization:
As sending event data to every master in dewrito.json would eat up bandwidth, a system for master servers to sync the events should be used instead.

When the player wants to send an event it goes through each master in the json file, but after one of them gives a successful return value the user can then stop sending to other masters.  
Once this master receives this data it then sends it to other masters registered in it's config as a "trusted" master server, the receiever master would then check if the sender is in it's "trusted" list, and if it is then the data will also be stored on this secondary server.  
This server can then send the data to the other "trusted" masters it knows about, allowing us to establish a network of master servers that know and trust each other not to send false info.  
Stats messages are signed anyway, so falsified info could be detected and discarded. The "trust" system is only needed so third-parties don't try sending fake (but signed properly) updates.

#### Master server key backup / optional user registration:
Instead of a traditional login/password system where the server would have to be sent the cleartext password in order to validate it (leaving a gap for MITM attacks to gain the password), I propose a different solution, allowing for any server (trusted or not) to store the users credentials:

Once the users keypair is generated the user is asked if they wish to backup this key online. To do so they are prompted for their email and password, however the cleartext password would never reach the server.  
Instead the private/public keys of the user would be encrypted with this password (possibly hashed/key-stretched), and the encrypted keypair + email are sent to each master.

This way the master would only store the users email and some data encrypted with their password, the password itself would never touch the server.  
If a hacker took control of the master server instead of a login database they'd find a database of email addresses and encrypted blobs.  
Depending on the encryption and keysize these blobs could take years to decrypt, compared to a typical database holding hashed passwords which would take a lot less computing power to attack.

When the user wants to recover their key they'd send a request to the masters containing their email address, the masters would then send the encrypted key blob to the users email, ensuring that the only people who would have access to this encrypted keyblob are the user and the master server operator.  
Once the user receives this keyblob they'd save it as "keyrestore.txt" in the DR folder. Once DR is started and detects the keyblob the user would be asked for the password, and if it's correct then the keyblob will decrypt successfully and the keys would be restored.

As said previously this way allows us to never let the master know our password at all - all encryption and decryption using the password key would take place on the client side.  
Even with the strongest password hashing algo if a hacker gained access to a server that uses traditional login/password it would still be simple for them to recover cleartext passwords, they'd just setup a MITM logger between the master and the internet.  
However with a solution like this they'd only gain the same data they'd get from the database, leaving hackers with data that's much harder to crack than a normal password hash.

This way is almost the same as other proposed methods involving optional registration, except now we can trust any master server with the credentials, as the credentials themselves aren't whats being sent.  
It also allows DR to remain neutral and not in control of any one party, players will also be able to use any name they wish as this system would be tied to their email address instead.  
One might argue that this stops us from banning users from the game, but no matter what we'd be unable to ban users, unless maybe we implemented some intruding hardware-based ID system, or a paid system (which is obviously out of the question)


