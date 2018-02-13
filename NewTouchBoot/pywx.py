#!/usr/bin/python
# -*- coding: utf-8 -*-

import wx
import subprocess
from os import system as fileopen
from time import sleep
width=0
height=0

def scale_bitmap(bitmap, widther, heighter):
    image = wx.ImageFromBitmap(bitmap)
    image = image.Scale(widther, heighter, wx.IMAGE_QUALITY_HIGH)
    result = wx.BitmapFromImage(image)
    return result


class Example(wx.Frame):
    global width
    global height       
    def __init__(self, *args, **kw):
        super(Example, self).__init__(*args, **kw) 
        
        self.InitUI()
	
    def InitUI(self):   
        global width
        global height
        pnl = wx.Panel(self)
        
        self.col = wx.Colour(0, 0, 0)

        rpbmp = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/retro.jpg", wx.BITMAP_TYPE_ANY)
        rpbmp = scale_bitmap(rpbmp, width/2, height/2)
        kbbmp = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/kodi.jpg", wx.BITMAP_TYPE_ANY)
        kbbmp = scale_bitmap(kbbmp, width/2, height/2)
        rbbmp = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/Debian.jpg", wx.BITMAP_TYPE_ANY)
        rbbmp = scale_bitmap(rbbmp, width/2, height/2)
        tbbmp = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/terminal.png", wx.BITMAP_TYPE_ANY)
        tbbmp = scale_bitmap(tbbmp, width/2, height/2)

        rpb = wx.BitmapButton(pnl, bitmap=rpbmp, pos=(0,0), size=(width/2,height/2))
        kb = wx.BitmapButton(pnl, bitmap=kbbmp, pos=(width/2, 0), size=(width/2,height/2))
        rb = wx.BitmapButton(pnl, bitmap=rbbmp, pos=(0, height/2), size=(width/2,height/2))
        tb = wx.BitmapButton(pnl, bitmap=tbbmp, pos=(width/2, height/2), size=(width/2,height/2))

        rpb.Bind(wx.EVT_BUTTON, self.Pressedrpb)
        kb.Bind(wx.EVT_BUTTON, self.Pressedkb)
        rb.Bind(wx.EVT_BUTTON, self.Pressedrb)
        tb.Bind(wx.EVT_BUTTON, self.Pressedtb)

        self.SetSize((width, height))
        self.SetTitle('Starting Your Entertainment Experience')
        self.Centre()
        self.Show(True)     

    def Pressedrpb(self, e):

        fileopen('/opt/retropie/configs/all/NewTouchBoot/mounts')
        sleep(2)
        f = open("/opt/retropie/configs/all/NewTouchBoot/checknum","w")
        f.write("num=1")
        f.close()
        exit()

    def Pressedkb(self, e):

        fileopen('/opt/retropie/configs/all/NewTouchBoot/mounts')
        sleep(2)
        f = open("/opt/retropie/configs/all/NewTouchBoot/checknum","w")
        f.write("num=2")
        f.close()
        exit()

    def Pressedrb(self, e):

        fileopen('/opt/retropie/configs/all/NewTouchBoot/mounts')
        sleep(2)
        f = open("/opt/retropie/configs/all/NewTouchBoot/checknum","w")
        f.write("num=3")
        f.close()
        exit()

    def Pressedtb(self, e):

        fileopen('/opt/retropie/configs/all/NewTouchBoot/mounts')
        sleep(2)
        f = open("/opt/retropie/configs/all/NewTouchBoot/checknum","w")
        f.write("num=4")
        f.close()
        exit()

def main():
    global width
    global height
    cmd = ['xrandr']
    cmd2 = ['grep', '*']
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    p2 = subprocess.Popen(cmd2, stdin=p.stdout, stdout=subprocess.PIPE)
    p.stdout.close()
    resolution_string, junk = p2.communicate()
    resolution = resolution_string.split()[0]
    strwidth, strheight = resolution.split('x')
    width=int(strwidth)
    height=int(strheight)
    ex = wx.App()
    Example(None)
    ex.MainLoop()    


if __name__ == '__main__':
    main()    

