# PiSwitch
Work on PiSwitch Project

Hi I'm building a repo for my PiSwitch project which is growing in popularity, feel free to use or expand upon what I'm working on here. I'm not a stickler about getting credit for my work but if you do anyting cool with it I'd love to see / here from you, especially if you've made improvements and I'm the first to admit there's lots of room for that.

In the the main root you have my autostart.sh fot Retropie. In NewTouchBoot are all of my startup files including an empty file called mount where you can add in any network mounts you have.  in Joymap you'll have my map files and the binary loadmap the compiled version of joymap 0.4.2. Joymap (A.K.A) Linux joystick mapper is not my software but I made modifications to the original developers source code to optimize it for use with the switch joycon controllers. Natively Linux Joystick Mapper did not support the analog stick's and only had button / d-pad support.

Most People came here from

https://www.instructables.com/id/PiSwitch/

if you have questions I'd look here first, I'm happy to help answer anything I can.

The map files are located in the joymap folde - loadmap is the binary ofr Linux Joystick Mapper recompiled for JoyCon support. You may need too recompile this for your device in that case go to the joymap-0.4.2 folder, delete loadmap run "make" and then copy the new loadmap over to the joymap directory.

NewTouchBoot is a folder that gets put under /opt/retropie/configs/all/  It contains a bunch of scripts and a python program that launches a touch boot option menu.
