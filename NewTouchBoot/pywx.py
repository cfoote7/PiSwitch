#!/usr/bin/python
# -*- coding: utf-8 -*-

from time import sleep
import wx
import subprocess
from threading import Thread
from os import system as fileopen
from time import sleep

width=0
height=0
jconr=False
jconl=False

class TestThread(Thread):
    global jconr
    global jconl
    #----------------------------------------------------------------------
    def run(self):
        global jconr
        global jconl
        while True:
            cmd = ['hcitool', 'con']
            cmd2 = ['grep', '']
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
            p2 = subprocess.Popen(cmd2, stdin=p.stdout, stdout=subprocess.PIPE)
            p.stdout.close()
            string, junk = p2.communicate()
            string=str(string)
            test = string.split("ACL ")
            for i in test:
                eval=i.split(" ")
                cmd=['hcitool', 'name', str(eval[0])]
                cmd2 = ['grep', '']
                p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
                p2 = subprocess.Popen(cmd2, stdin=p.stdout, stdout=subprocess.PIPE)
                p.stdout.close()
                ns, junk = p2.communicate()
                ns=str(ns.strip())
                i=ns
                if i == "Joy-Con (R)":
                    if jconr == False:
                        jconr=True
                if i == "Joy-Con (L)":
                    if jconl == False:
                        jconl=True
            if jconr is True:
                if jconl is True:
                    break

def scale_bitmap(bitmap, widther, heighter):
    image = wx.ImageFromBitmap(bitmap)
    image = image.Scale(widther, heighter, wx.IMAGE_QUALITY_HIGH)
    result = wx.BitmapFromImage(image)
    return result        
        
class PanelOne(wx.Panel):
    """"""
    global width
    global height
    global jconr
    global jconl
    def __init__(self, parent):
        """Constructor"""
        wx.Panel.__init__(self, parent=parent)

class PanelTwo(wx.Panel):
    """"""
    global width
    global height
    global jconr
    global jconl
    def __init__(self, parent):
        """Constructor"""
        wx.Panel.__init__(self, parent=parent)

class PanelThree(wx.Panel):
    """"""
    global width
    global height
    def __init__(self, parent):
        """Constructor"""
        wx.Panel.__init__(self, parent=parent)

class MyForm(wx.Frame):
    global jconl
    global jconr
    global width
    global height

    def __init__(self, *args, **kw):
        super(MyForm, self).__init__(*args, **kw) 
        
        self.InitUI()

    def InitUI(self):  
        global jconr
        global jconl
        self.panel_two = PanelTwo(self)
        self.panel_one = PanelOne(self)
        self.panel_three = PanelThree(self)
        self.SetSize((width, height))
        self.SetTitle('Starting Your Entertainment Experience')
        self.Centre()
        self.panel_one.SetSize((width, height))
        self.panel_two.SetSize((width, height))
        self.panel_three.SetSize((width, height))
        self.panel_one.Hide()
        self.panel_three.Hide()
        self.panel_two.SetBackgroundColour(wx.WHITE)
        
        right = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/right.jpg", wx.BITMAP_TYPE_ANY)
        self.panel_two.rcon = wx.StaticBitmap(self.panel_two, id=wx.ID_ANY, bitmap=right, pos=(width/2 + 220, 100))
        left = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/left.jpg", wx.BITMAP_TYPE_ANY)
        self.panel_two.lcon = wx.StaticBitmap(self.panel_two, id=wx.ID_ANY, bitmap=left, pos=(width/2 - 350, 100))
        self.panel_two.button = wx.Button(self.panel_two, wx.ID_ANY, 'Skip', pos=(width/2 - 50, height/2 + 120), size=(100, 50))
        self.panel_two.button.Bind(wx.EVT_BUTTON, self.PressedButton)
        
        self.panel_two.lbl = wx.StaticText(self.panel_two, wx.ID_ANY, pos=(width/2 - 225, 10), style = wx.ALIGN_CENTER)
        txt1 = "To Connect Your Joy-Con Controllers" 
        txt2 = "Press the Plus and Minus Buttons" 
        txt3 = "A Check Box Will Appear to Indicate Success." 
        txt = txt1+"\n"+txt2+"\n"+txt3
        font = wx.Font(18, wx.ROMAN, wx.ITALIC, wx.NORMAL)
        self.panel_two.lbl.SetFont(font)
        self.panel_two.lbl.SetLabel(txt)

        check = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/check.png", wx.BITMAP_TYPE_ANY)
        check = scale_bitmap(check, 50, 50)
        self.panel_two.checkconr = wx.StaticBitmap(self.panel_two, id=wx.ID_ANY, bitmap=check, pos=(width/2 + 250, 30))
        self.panel_two.checkconl = wx.StaticBitmap(self.panel_two, id=wx.ID_ANY, bitmap=check, pos=(width/2 - 310, 30))
        self.panel_two.checkconl.Hide()
        self.panel_two.checkconr.Hide()
        
        self.panel_three.OnePlayer = wx.Button(self.panel_three, wx.ID_ANY, 'One Player', pos=(width/2 - 300, height/2 - 150), size=(600, 150))
        self.panel_three.OnePlayer.SetFont(font)
        self.panel_three.OnePlayer.Bind(wx.EVT_BUTTON, self.PressedOnePlayer)
        self.panel_three.TwoPlayers = wx.Button(self.panel_three, wx.ID_ANY, 'Two Players', pos=(width/2 - 300, height/2), size=(600, 150))
        self.panel_three.TwoPlayers.SetFont(font)
        self.panel_three.TwoPlayers.Bind(wx.EVT_BUTTON, self.PressedTwoPlayers)
        
        rpbmp = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/retro.jpg", wx.BITMAP_TYPE_ANY)
        rpbmp = scale_bitmap(rpbmp, width/2, height/2)
        kbbmp = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/kodi.jpg", wx.BITMAP_TYPE_ANY)
        kbbmp = scale_bitmap(kbbmp, width/2, height/2)
        rbbmp = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/Debian.jpg", wx.BITMAP_TYPE_ANY)
        rbbmp = scale_bitmap(rbbmp, width/2, height/2)
        tbbmp = wx.Bitmap("/opt/retropie/configs/all/NewTouchBoot/terminal.png", wx.BITMAP_TYPE_ANY)
        tbbmp = scale_bitmap(tbbmp, width/2, height/2)

        self.panel_one.rpb = wx.BitmapButton(self.panel_one, bitmap=rpbmp, pos=(0,0), size=(width/2,height/2))
        self.panel_one.kb = wx.BitmapButton(self.panel_one, bitmap=kbbmp, pos=(width/2, 0), size=(width/2,height/2))
        self.panel_one.rb = wx.BitmapButton(self.panel_one, bitmap=rbbmp, pos=(0, height/2), size=(width/2,height/2))
        self.panel_one.tb = wx.BitmapButton(self.panel_one, bitmap=tbbmp, pos=(width/2, height/2), size=(width/2,height/2))
        
        self.panel_one.rpb.Bind(wx.EVT_BUTTON, self.Pressedrpb)
        self.panel_one.kb.Bind(wx.EVT_BUTTON, self.Pressedkb)
        self.panel_one.rb.Bind(wx.EVT_BUTTON, self.Pressedrb)
        self.panel_one.tb.Bind(wx.EVT_BUTTON, self.Pressedtb)
        
        self.panel_two.timer = wx.Timer(self.panel_two, wx.ID_ANY)
        # update clock digits every second (1000ms)
        self.panel_two.Bind(wx.EVT_TIMER, self.OnTimer)
        self.panel_two.timer.Start(100)
        #self.Centre()   

        self.SetSize((width, height))
        self.SetTitle('Starting Your Entertainment Experience')
        self.Centre()
        self.Show(True)

    def OnTimer(self, event):
        global jconr
        global jconl
        if jconr is True:
            self.panel_two.checkconr.Show()
        if jconl is True:
            self.panel_two.checkconl.Show()
            if jconr is True:
                self.panel_two.timer.Stop()
                self.panel_two.button.SetLabel("Next")
            
    def PressedButton(self, e):
        global jconr
        global jconl
        jconr=True
        jconl=True
        self.panel_two.timer.Stop()
        self.panel_two.Hide()
        self.panel_one.Show()
        
    def Pressedrpb(self, e):
        self.panel_one.Hide()
        self.panel_three.Show()
        
    def PressedOnePlayer(self, e):
        fileopen('/opt/retropie/configs/all/NewTouchBoot/mounts')
        sleep(0.1)
        f = open("/opt/retropie/configs/all/NewTouchBoot/checknum","w")
        f.write("num=1")
        f.close()
        exit()
        
    def PressedTwoPlayers(self, e):
        fileopen('/opt/retropie/configs/all/NewTouchBoot/mounts')
        sleep(0.1)
        f = open("/opt/retropie/configs/all/NewTouchBoot/checknum","w")
        f.write("num=5")
        f.close()
        exit()

    def Pressedkb(self, e):

        fileopen('/opt/retropie/configs/all/NewTouchBoot/mounts')
        sleep(0.1)
        f = open("/opt/retropie/configs/all/NewTouchBoot/checknum","w")
        f.write("num=2")
        f.close()
        exit()

    def Pressedrb(self, e):

        fileopen('/opt/retropie/configs/all/NewTouchBoot/mounts')
        sleep(0.1)
        f = open("/opt/retropie/configs/all/NewTouchBoot/checknum","w")
        f.write("num=3")
        f.close()
        exit()

    def Pressedtb(self, e):

        fileopen('/opt/retropie/configs/all/NewTouchBoot/mounts')
        sleep(0.1)
        f = open("/opt/retropie/configs/all/NewTouchBoot/checknum","w")
        f.write("num=4")
        f.close()
        exit()
 
    #----------------------------------------------------------------------

 
# Run the program
if __name__ == "__main__":
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
    MyForm(None)
    TestThread().start()
    ex.MainLoop() 