# TWatchSK

TWatchSK gives you a way to wear [Signal K](https://github.com/SignalK) on your wrist! It connects via wifi to your Signal K Server and displays whatever SK Paths you choose.
Perhaps more important, it receives SK Notifications so that you know immediately when something important is going on with your boat, no matter
where you are on the boat or what you're doing.

It runs exclusively on the [LILYGO T-Watch-2020 watch](http://www.lilygo.cn/prod_view.aspx?TypeId=50053&Id=1290&FId=t3:50053:3), which is based on an ESP32 microcontroller.

![image](https://user-images.githubusercontent.com/15186790/124196885-895f5380-da8a-11eb-9838-c23fe4748867.png)![image](https://user-images.githubusercontent.com/15186790/124197236-56698f80-da8b-11eb-9484-81c49a57b7c2.png)

The code is C++, built with PlatformIO and VS Code. The source code is open, under the (Jan: which open source license?).

## Features

- Main watch screen shows hours, minutes, and seconds in 12-hour or 24-hour format. Also shows the day of the week and the date.
- Icons indicate the status of the wifi and Signal K Server connections, and the battery status.
- Easy setup screens to set date and time, wifi credentials, Signal K connection details, display options, and "wake-up" options.
- User-defined screens to show current data from any SK Path(s), with multiple layout options.
- Automatic display of any notifications sent by Signal K. Notifications "wake up" the watch, vibrate the watch, and (optionally) play a sound file.
- Designed for long battery life - easily 24 hours between charges.
- Watch can "wake up" with a double-tap to the screen, or simply by bringing your wrist into the "look at the watch" position.
- When awake, a double-tap switches between day and night mode.
- Easy to adapt to almost any language.
- Step counter (steps since you last restarted the watch) displayed in the upper left corner of the main screen.

## Operation
- Turn the watch on: push and hold the external button for about 3 seconds, then release. The main screen should appear a few seconds later.
- Turn the watch off: push and hold the external button for at least 6 seconds, and the screen will go off. To make sure it's really off, push
and release the external button quickly. If it comes on immediately, you didn't really turn it off - it just went to sleep at the end of the normal
screen timeout time. Try again, making sure you hold the button in for at least 6 full seconds. When the watch is off, it's not performing any
function other than keeping the date and time, so the battery life is greatly extended.
- Put the watch to sleep: push and release the external button, and the screen will go blank. Alternatively, it will automatically go blank after
the number of seconds you specify in the "Screen timeout" field on the Display Settings screen. When the watch is asleep, it's still connected to
wifi and your Signal K Server, and will automatically wake up if any SK notifications come in, or if you lose connection to wifi or the SK Server.
- Wake up the watch: push and release the external button and the screen will come on, and will stay on for the number of seconds you specify in
the "Screen timeout" field on the Display Settings screen. If you want the screen to wake up for only a couple of seconds, so you can check the time,
you can double-tap the screen, or tilt the watch into a normal "look at the watch" position. Each of these methods works only if they are enabled
on the "Wakeup Settings" screen, accessed from the main menu. The double-tap needs to be quick and firm. The "tilt" move is something you may need
to practice: start with your arm hanging at your side, and smoothly bring your hand up in front of you so that you're looking at the face of the watch.
- Switch between day and night mode: double-tap the screen when the watch is awake. If you wake the watch up with a double-tap or a tilt, and you can't
read the screen because it's in the wrong mode (day vs. night) for your current lighting conditions, you can quickly double-tap it again, which will
switch from day to night (or vice versa), and will begin the normal "Screen timeout" timer for leaving the screen on.
- To see your Signal K "DynamicViews" screens: from the main screen (the screen that display the time and date), swipe left to see the first DynamicView
screen, and keep swiping left to see each of them in turn. From any DynamicView screen, swipe right until you're back on the main screen. NOTE: you won't have any SK
DynamicView screens on your watch until you set them up - see below for details.

## Inital Setup

Once you have the software loaded (BS: link here to something with those details) and the watch boots, you'll see the main screen, showing the time and date.
Those will almost certainly be wrong, so the first thing to do is set them correctly.

### Date and Time
- Tap the "four squares" icon, which takes you into the menu system.
- Tap the "Clock" menu item and you'll see the "Time settings" screen.
- Select the proper setting for "Show 24 hour time".
- Tap the first field - the "hour" field - and the screen changes to a keyboard with numbers. 
- Enter the current hour, then tap the checkmark key. (The "Del" key can be used to backspace over whatever you have already entered.)
- The screen will change back to the "Time settings" screen, with the hour correctly set.
- Tap the next field - the "minutes" field - and use the keyboard to set the current minute.
- If you are NOT showing 24 hour time, you will have an "am/pm" field - tap it to toggle between those two.
- Tap the date field, and the screen changes to a calender. Tap on the date and the calendar will close.
- You can set the timezone, but it is not currently used for anything.
- Similarly, the "Sync time with SK Server" button is not currently used.
- Tap the left arrow in the upper left corner of screen to return to the "Watch settings" menu.

You can now tap the "Back" button (the arrow in the upper left corner) to return to the main screen, to make sure you have the date and time set correctly.
Or you can continue with the setup process. Each of the menu items have at least one field for you to setup. Each is detailed below.

### Display
- "Screen timeout" is the number of seconds that the screen will stay on with no activity. A shorter screen timeout will use less battery than a long timeout,
but unless you turn the watch on very frequently, it probably won't make much difference in overall battery life. The default value is 30 seconds, but you can
make it whatever you like, up to 99 seconds. (Note: when you wake up the screen with a double-tap or a tilt, it is assumed you are only looking at the time, so
the screen timeout is temporarily set to 2 seconds. That has no effect on the normal screen timeout setting.)
- "Display brightness" sets the brightness level of the screen. 1 is the least bright, and is the setting for the "Dark theme" (most commonly called "night mode").
5 is the brightest, and is the setting the watch goes to if you double-tap to switch from night mode to day mode. So, while you can set the display brightness
to 2, 3, or 4, the setting will be lost if you ever use the double-tap to switch from night mode to day mode. In practice, there is probably no need for
these "middle" settings, but they were added before night mode existed, and they have been left in place.
- "Enable Dark theme" toggles between day mode and night mode. In practice, there is no need for this switch, but it was added before the double-tap that toggles
between day mode and night mode, and it has been left in place.
- "Download DynamicViews": this is how you install the DynamicViews (the screens that display Signal K data) that you define in the DynamicViews Editor, which
is a program you run on your computer. The Editor allows you to create the DynamicViews and saves them on your Signal K Server, then this button installs
them onto the watch.

### Wifi
The first time you go to the "Wifi settings" screen, you'll see this (BS: show the screenshot), indicating that you need to configure the wifi settings
- Tap the "Connect" button and the watch will scan and display any in-range wifi networks. The strongest signal will be at the top of the list, the weakest
signal at the bottom. Tap on the network you want (this has to be the same network that your Signal K Server is on!), and then enter the password.
- Save the password (tap the checkmark key), and the watch will attempt to connect. If it succeeds, you'll see the "Success!" message, and you should never
have to configure wifi again, unless you change the network name or the password. After you tap "OK" to close the "Success!" message, you will see the name
of the network, the word "Connected", the IP address that has been assigned to the watch by your DHCP server, and you'll see the wifi signal strength.
- If you get a message that the watch can't connect to wifi, it is almost certainly because you entered the wrong password. Close the message screen,
then tap the "Scan" button, select the network again, and enter the password again.
- If you're going to be away from the boat and you want to disable wifi, you can do it from this screen. Or you can just wait until you see a message
indicating that the watch can't connect to wifi, and tap the "Disable wifi" button on that message.
- When wifi is disabled, you can come to this screen, make sure the correct wifi network name is displayed, and tap the "Connect" button, which will
enable wifi and attempt to connect.
- If you ever change the name of your network, you can come to this screen and (assuming you're not connected to wifi) tap the "Scan" button to select
the new network.
- On the main watch screen, there is a wifi icon in the upper right corner if wifi is enabled. The icon is black if wifi is connected, and red if it
is not connected.

### Signal K
(NOTE: Before you connect to the SK Server the very first time, you might want to set your watch's name, on the Watch Info menu, described below. You don't
have to, but if you don't, you'll have to re-authorize the watch's SK Server access when you do change the watch's name.)
This screen shows the current status of the connection to the SK Server (shows the server address and the SK Server version number if connected), and
provides buttons for managing the connection to the SK Server. Note that you can make the initial connection to the Server by specifically entering
its IP address and port number, or by using the "Find SK Server w/ mDNS" button: both methods are described below.
- "Set address": tap to enter the IP address, or the hostname, of your SK Server. NOTE: the watch and the SK Server must be on the same wifi network.
- The field to the right of the address is the "Port" field - it's just not labeled. Tap it to enter the port of your SK Server: typically 80 or 3000.
BS: write the description of what happens after you set the address and port: does it automatically try to connect after you save either of those fields?
- If you don't know the IP address of your SK Server, or if you just want to save some tapping, use this button to search the network for the Server.
Jan: Does it work?
- Once you're connected to the SK Server, you'll need to go to the Server from a browser and authorize access for the watch, so that they can communicate
properly. (BS: provide more details here)
- BS: write up the "Reset token" button - not sure when you would use it. Jan: when would you use it?

### Wake-up
As described above, you can wake the watch up three different ways:
- A press-and-release of the external button wakes the watch up and leaves the screen on for the normal "Screen timeout" number of seconds.
- A double-tap or a "tilt" wake the watch up for just 2 seconds, so you can check the time.
But neither the double-tap nor the tilt methods are enabled by default. Since they can happen accidentally, and you may not want the watch to ever
wake up accidentally, you must enable them or they won't work. You enable each of them from this screen.

### Watch info
This screen mostly shows some basic info about the watch and the software, but it does have one editable field: "Watch name". Tap it to bring up a
keyboard and enter the name you want for the watch. It will be displayed above the time on the main screen, and you'll see it in the SK Server, associated
with the data that is sent to the Server (watch battery status, etc.) After you change the name of the watch, you'll need to authorize its access on
the SK Server. 

BS: To Do
- Add images to the text above.
- Add a section about the DynamicViews, and a link to the DynamicViews Editor documentation.
- Get Jan to make a short video showing the main functions of the watch, including some DynamicViews and the display of an SK Notification.
