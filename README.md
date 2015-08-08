![Eldewrito Logo](https://halo.click/H4STD8)
# DewRecode - A recode of ElDewrito

## What is ElDewrito?
ElDewrito is a fan mod for Halo Online that re-enables removed Halo 3 content, as there is no telling if Halo 3 will ever be brought to PC.

Halo Online is the closest thing we have, and fortunately it still contains a lot of Halo 3 content inside it. ElDewrito aims to unlock and expand on this content.

Note that this is the source code repo for it, not the mod itself. You'll need a copy of Halo Online and the ElDewrito Launcher to install ED.

Pull requests are welcomed from anyone who wants to contribute to us.  
Although it's recommended that you come talk to us on IRC first before starting work on anything major, so that we can discuss it and come up with the best way to help you implement it.

## Download
You can always download the latest builds of Eldewrito from our [release page](https://dewrito.halo.click/).

## Building
To build ElDewrito you'll need Visual Studio 2013 and the DirectX June 2010 SDK installed.

Open DewRecode.sln and build the whole solution, sit back and relax while it builds since it may take a while.

If you don't have your Halo Online installation inside C:\Halo\ you may get a post-build error that mentions being unable to copy.

You can ignore that, and follow the running instructions below.

## Running
To run DewRecode you should start off with a fresh Halo Online (21.03) install, without the older ElDewrito or any other mods applied.

However you will need the patched eldorado.exe and maps\tags.dat files from an ElDewrito install in order for DewRecode to load properly.

DewRecode may have issues with the binks used by HO, so you should also either remove or rename your bink folder.

Once DewRecode is built simply copy the built mtndew.dll to your HO folder.

Any built plugins should also be placed in the mods\plugins\ folder inside your HO install.

The files in the DewRecode\dist\ folder in this git repo should also be copied to the root of your HO install, as they setup default bindings and initial setup.

## Donations
We don't accept donations, donate money to your favorite charity instead.

We do it for free, because the possibility of Halo 3 on PC fills us with glee.

Although if you have a spare server running [ElDewrito-MasterServer](https://github.com/ElDewrito/ElDewrito-MasterServer) on it would be welcomed, get in touch with us on IRC and we can help you set this up.

## Help/Support/Contact
We have an [IRC channel on Snoonet](https://irc.lc/snoonet/eldorito/) where most of the devs are, just /join #ElDorito

You can also try checking the support question on the [Halo.Click](https://forum.halo.click/index.php?/forum/17-help-support/) forums. 

If you have issues you can get in touch with us on there, or make a bug report in our issue tracker and we'll look over it.