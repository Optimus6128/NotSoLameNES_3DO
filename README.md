
= NotSoLameNES for 3DO =

This is an attempt to radically change and optimize the original LameNES 3DO port.
I was working with this on a fork, but since I realized I will be masively changing the original code to make it possible to run not under 1FPS on real 3DO, and will only use it as a base, it will stop resembling the original code, so why not make it into a new repository? I've already massacred the PPU and in some future plans I might even replace it with CEL rendering. I identified a lot of places, also some on CPU, that this could be written entirely different to just get that pipedream speed up. Also, I don't like that commits on fork don't show on your calendar, as I seem to suddenly be so invested on this project and I like to show some activity. Changes will be massive, totally a new project, so easier from a new repo than fork and pull request.

= LameNES for 3DO =

LameNES by Joey Loman, licensed under a 2-clause BSD license.

Port by gameblabla (with help from Saffron)

LameNES for the 3DO.

It is very slow but it is working.

Credits to Saffron for making it to work. (without a debugger !)

To run, you must put a NES rom file in the CD folder and pack everything with OperaFS.

Here are the controls :

Dpad            =  NES Dpad
C               =  Select
Pause           =  Start
A               =  A
B               =  B

