# PiSwitch

---

    UPDATE-- v1.2 --2/19/18

---

    added controller pairing menu with instructions

    added debian on screen keyboard Menu > Accesories > Keyboard

    added one player / two player selection

---


# Work on PiSwitch Project

Hi I'm building a repo for my PiSwitch project which is growing in popularity, feel free to use or expand upon what I'm working on here. I'm not a stickler about getting credit for my work but if you do anyting cool with it I'd love to see / here from you, especially if you've made improvements and I'm the first to admit there's lots of room for that.

In the the main root you have my autostart.sh fot Retropie. In NewTouchBoot are all of my startup files including an empty file called mount where you can add in any network mounts you have.  in Joymap you'll have my map files and the binary loadmap the compiled version of joymap 0.4.2. Joymap (A.K.A) Linux joystick mapper is not my software but I made modifications to the original developers source code to optimize it for use with the switch joycon controllers. Natively Linux Joystick Mapper did not support the analog stick's and only had button / d-pad support.

Most People came here from

https://www.instructables.com/id/PiSwitch/

if you have questions I'd look here first, I'm happy to help answer anything I can.

The map files are located in the joymap folde - loadmap is the binary ofr Linux Joystick Mapper recompiled for JoyCon support. You may need too recompile this for your device in that case go to the joymap-0.4.2 folder, delete loadmap run "make" and then copy the new loadmap over to the joymap directory.

NewTouchBoot is a folder that gets put under /opt/retropie/configs/all/  It contains a bunch of scripts and a python program that launches a touch boot option menu.

Tips:

The controllers map to keyboard keys that's how I got both controllers working as one controller. I'm building solutions to connect controllers more smoothly and to choose single or two palyer modes. If you boot up and choose retropie without pressing any controller buttons you can setup the two controllers seperately as single player controllers. then anytime you boot up without pressing any buttons to wake the remotes they'll work as 2 seperate players. also if you ever get stuck with no controllers working and want to navigate retropie without a joy-con the arrow keys are the d-pad "A" is "v", "B" is "x", "start" is "enter" and "select" is "shift"
