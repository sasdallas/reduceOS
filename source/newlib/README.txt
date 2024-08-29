A stripped-down version of newlib for reduceOS.

** IMPORTANT NOTE TO THOSE TRYING TO MAKE CHANGES: **
Newlib itself is very finnicky to work with. The code requires you have specific versions of autoconf and automake.
Here are my tested and known good versions for building this (the config files have already been generated):
- autoconf 2.69
- automake 1.13.4

You may notice that Newlib's README in its directory says to use different versions. News flash:
Newlib devs don't understand autoconf and automake. Support for Cygwin buildtrees was removed way long ago
Now, to give them some MUCH deserved credit, newlib is absolutely AMAZING. I love everything about it.


Also, for anyone curious - the version of newlib in reduceOS is:
3.0.0.20180226

