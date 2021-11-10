# TWatchSK

TWatchSK gives you a way to wear [Signal K](https://github.com/SignalK) on your wrist! It connects via wifi to your Signal K Server and displays whatever SK Paths you choose.
Perhaps more important, it receives SK Notifications so that you know immediately when something important is going on with your boat, no matter
where you are on the boat or what you're doing.

It runs exclusively on the [LILYGO T-Watch-2020 watch](http://www.lilygo.cn/prod_view.aspx?TypeId=50053&Id=1290&FId=t3:50053:3), which is based on an ESP32 microcontroller. (BAS: Any limits on which version of the T-Watch 2020 it runs on?)

![image](https://user-images.githubusercontent.com/15186790/125843967-e44c8015-d0a8-4099-8e00-b63a9b4c29cb.png) ![image](https://user-images.githubusercontent.com/15186790/125844616-d3264ca2-3667-4789-9cda-456e79ba2aa9.png) ![image](https://user-images.githubusercontent.com/15186790/140789761-97ee13f6-3feb-4521-9606-0127ba0ffb9a.png)


The code is C++, built with PlatformIO and VS Code. The source code is open, under the (BAS: which open source license?).

## Features

- Main watch screen (called the home screen) shows hours, minutes, and seconds in 12-hour or 24-hour format. Also shows the day of the week and the date.
- Icons on the status bar indicate the status of the wifi and Signal K Server connections, and the battery status, and the count of unread messages.
- Easy setup screens to set date and time, wifi credentials, Signal K connection details, display options, and "wake-up" options.
- User-defined screens (called DynamicViews) to show current data from any SK Path(s), with multiple layout options.
- Automatic display of any notifications sent by Signal K. Notifications "wake up" the watch, vibrate the watch, and play a sound file.
- Designed for long battery life - you can easily go 24 hours between charges.
- Watch can "wake up" with a double-tap to the screen, a press-and-release on the screen, or simply by bringing your wrist into the "look at the watch" position.
- When awake, a double-tap switches between day and night mode.
- Easy to adapt to almost any language.
- Step counter (steps since you last restarted the watch) displayed in the upper left corner of the home screen.

## Operation
- Turn the watch on: push and hold the external button for about 3 seconds, then release. The home screen should appear a few seconds later.
- Turn the watch off: push and hold the external button for at least 6 seconds, and the screen will go off. To make sure it's really off, push
and release the external button quickly. If it comes on immediately, you didn't really turn it off - it just went to sleep at the end of the normal
screen timeout time. Try again, making sure you hold the button in for at least 6 full seconds. When the watch is off, it's not performing any
function other than keeping the date and time, so the battery life is greatly extended.
- Put the watch to sleep: push and release the external button, and the screen will go blank. Alternatively, it will automatically go blank after
the number of seconds you specify in the "Screen timeout" field on the Display Settings screen. When the watch is asleep, it's still connected to
wifi and your Signal K Server, and will automatically wake up if any SK notifications come in, or if you lose connection to wifi or the SK Server.
- Wake up the watch: push and release the external button and the screen will come on, and will stay on for the number of seconds you specify in
the "Screen timeout" field on the Display Settings screen. If you want the screen to wake up for only a couple of seconds so you can check the time,
you can double-tap the screen, press-and-release on the screen, or tilt the watch into a normal "look at the watch" position. (Each of these methods works only if they are enabled
on the "Wakeup Settings" screen, accessed from the main menu.) The double-tap needs to be quick and firm. The "tilt" move is something you may need
to practice: start with your arm hanging at your side, and smoothly bring your hand up in front of you so that you're looking at the face of the watch.
- Switch between day and night mode: double-tap the screen when the watch is awake. If you wake the watch up with a double-tap, press-and-release, or a tilt, and you can't
read the screen because it's in the wrong mode (day vs. night) for your current lighting conditions, you can quickly double-tap it again, which will
switch from day to night (or vice versa), and will begin the normal "Screen timeout" timer for leaving the screen on.
- To see your Signal K "DynamicViews" screens: from the home screen (the screen that display the time and date), swipe left to see the first DynamicView
screen, and keep swiping left to see each of them in turn. From any DynamicView screen, swipe right until you're back on the home screen. NOTE: you won't have any SK
DynamicView screens on your watch until you set them up - see below for details.

## Inital Setup

Once you have the software loaded - [click here for instructions](https://github.com/JohnySeven/TWatchSKDesigner/blob/master/README.md#installing-twatchsk) and the watch boots, you'll see the home screen, showing the time and date.
Those will almost certainly be wrong, so the first thing to do is set them correctly.

### Date and Time
- Tap the "four squares" icon, which takes you into the menu system.
- Tap the "Clock" menu item and you'll see the "Time settings" screen.
- Select the desired setting for "Show 24 hour time".
- Tap the first field - the "hour" field - and the screen changes to a keyboard with numbers. 
- Enter the current hour, then tap the checkmark key. (The "Del" key can be used to backspace over whatever you have already entered.)
- The screen will change back to the "Time settings" screen, with the hour correctly set.
- Tap the next field - the "minutes" field - and use the keyboard to set the current minute.
- If you are NOT showing 24 hour time, you will have an "am/pm" field - tap it to toggle between those two.
- Tap the date field, and the screen changes to a calender. Tap on the date and the calendar will close.
- You can set the timezone, but it is not currently used for anything.
- Similarly, the "Sync time with SK Server" button is not currently used.
- Tap the left arrow in the upper left corner of the screen to return to the "Watch settings" menu.

You can now tap the "Back" button (the arrow in the upper left corner) to return to the home screen, to make sure you have the date and time set correctly.
Or you can continue with the setup process. Each of the menu items have at least one field for you to setup. Each is detailed below.

### Display
- "Screen timeout" is the number of seconds that the screen will stay on with no activity. A shorter screen timeout will use less battery than a long timeout,
but unless you turn the watch on very frequently, it probably won't make much difference in overall battery life. The default value is 30 seconds, but you can
make it whatever you like, up to 99 seconds. (Note: when you wake up the screen with a double-tap, press-and-release, or a tilt, it is assumed you are only looking at the time, so
the screen timeout is temporarily set to 2 seconds. That has no effect on the normal screen timeout setting.)
- "Display brightness" sets the brightness level of the screen. 1 is the least bright, and is the setting for the "Dark theme" (called "night mode").
5 is the brightest, and is the setting the watch goes to if you double-tap to switch from night mode to day mode. So, while you can set the display brightness
to 2, 3, or 4, the setting will be lost if you ever use the double-tap to switch from night mode to day mode. In practice, there is probably no need for
these "middle" settings, but they were added before night mode existed, and they have been left in place. (BAS: ask Jan if we should just remove them.)
- "Enable Dark theme" toggles between day mode and night mode. In practice, there is no need for this switch, but it was added before the double-tap that toggles
between day mode and night mode, and it has been left in place. (BAS: Ask Jan if we should remove this switch.)
- "Download DynamicViews": this is how you install the DynamicViews (the screens that display Signal K data) that you define with [TWatchSKDesigner](https://github.com/JohnySeven/TWatchSKDesigner#twatchskdesigner), which
is a program you run on your computer. It allows you to create DynamicViews and saves them on your Signal K Server, then this button installs
them onto the watch.

### Wifi
The first time you go to the "Wifi settings" screen, you'll see this, indicating that you need to configure the wifi settings.

![image](https://user-images.githubusercontent.com/15186790/125854000-453d08a7-3ca7-409f-8393-9acb38f5f380.png)
- Tap the "Connect" button and the watch will scan and display any in-range wifi networks. The strongest signal will be at the top of the list, the weakest
signal at the bottom. (If you have more than one wifi access point on your network, you will see all of them listed, and the one with the strongest signal will be at the top of the list.) Tap on the network you want (this has to be the same network that your Signal K Server is on!), and then enter the password.
- Save the password (tap the checkmark key), and the watch will attempt to connect. If it succeeds, you'll see the "Success!" message, and you should never
have to configure wifi again, unless you change the network name or the password. After you tap "OK" to close the "Success!" message, you will see the name
of the network, the word "Connected", the IP address that has been assigned to the watch by your DHCP server, and you'll see the wifi signal strength.
- If you get a message that the watch can't connect to wifi, it is almost certainly because you entered the wrong password. Close the message screen,
then tap the "Scan" button, select the network again, and enter the password again.
- If you're going to be away from the boat and you want to disable wifi, you can do it from this screen. Or you can just wait until you see a message
indicating that the watch can't connect to wifi, and tap the "Disable Wifi" button on that message.
- When wifi is disabled, you can come to this screen, make sure the correct wifi network name is displayed, and tap the "Connect" button, which will
enable wifi and attempt to connect.
- If you ever change the name of your network, you can come to this screen and (assuming you're not connected to wifi) tap the "Scan" button to select
the new network.
- On the watch home screen and at the top of all DynamicView screens, there is a wifi icon in the upper right corner if wifi is enabled. The icon is black if wifi is connected, and red if it is not connected.

### Signal K
This screen shows the current status of the connection to the SK Server (shows the server address and the SK Server version number if connected), and
provides buttons for managing the connection to the SK Server.
- "Set address": tap to enter the IP address, or the hostname, of your SK Server. NOTE: the Watch and the SK Server must be on the same wifi network.
- The field to the right of the address is the "Port" field - it's just not labeled. Tap it to enter the port of your SK Server: typically 80 or 3000. As long as you're connected to wifi already, the Watch will attempt to connect to the SK Server. If it's successful, you'll see the Server address and version, and above that, it will say "Pending authorization".
- Login to your SK Server, click on "Security" on the left-side menu, then click on "Access requests", then click on TWatchSK in the list of Access Requests, then enter `NEVER` in the "Authentication Timeout" field, then click the "Approve" button.

![image](https://user-images.githubusercontent.com/15186790/125855553-ee8e067f-7ebe-42eb-a264-85a69129b066.png)
- The Watch should now show "Connected" where it previously showed "Pending authorization".
- When you return to the home screen, you should see a Signal K icon in the upper right, and it should be black. If it's red, that means there is a problem with the connection to the Signal K Server (including the "Pending authorization" status, until you approve it).
- The `Find SK Server w/ mDNS` button is a work-in-progress and currently does nothing.
- The `Reset token` button is used if you ever change the IP address, hostname, or port of your SK Server. If you do that, first make the change to those fields on this screen, and then tap the `Reset token` button. The Watch should reconnect with the SK Server, and then you'll need to approve a new access request on the Server, as described above.

### Wake-up
As described above, you can wake the watch up three different ways:
- A press-and-release of the external button wakes the watch up and leaves the screen on for the normal "Screen timeout" number of seconds.
- A double-tap, press-and-release, or a "tilt" wake the watch up for just 2 seconds, so you can check the time.
The double-tap, press-and-release (called "Screen touch" on this screen), nor the tilt methods are enabled by default. Since they can happen accidentally, and you may not want the watch to ever
wake up accidentally, you must enable them or they won't work. You enable each of them from this screen.

### Watch info
This screen mostly shows some basic info about the watch and the software, but it does have one editable field: "Watch name". Tap it to bring up a
keyboard and enter the name you want for the Watch. It will be displayed above the time on the home screen, and you'll see it in the SK Server, in the "path" of the data that is sent to the Server (watch battery status, etc.) (BAS: If you change the name, I think you have to re-do the security authorization.)

## Signal K Notifications
Signal K broadcasts notifications for various things: some are enabled by default (such as when a new version is available), but most are user-defined (such as when the value of a particular path is outside of a defined range, like engine oil pressure being too low). The watch receives all SK notifications, and if they are categorized as "warn", "alert", "alarm", or "emergency", the watch will wake up, display the notification as a message, vibrate, and play a sound.

![image](https://user-images.githubusercontent.com/15186790/124197236-56698f80-da8b-11eb-9484-81c49a57b7c2.png)

It's up to you to set up the notifications you want to see in Signal K, and give them the proper status: "warn", "alert", "alarm", or "emergency". The watch will take it from there. (The Simple Notifications plug-in is an easy way to set up notifications. There are other ways, too.)

## DynamicViews
The whole point of creating TWatchSK was to allow it to keep you informed of what's going on with your boat, by monitoring and displaying what's happening in Signal K. That's done with Notifications (described above) and DynamicViews. A DynamicView is a user-defined screen that can display data from any Signal K Path. In fact, each DynamicView screen can show data from one to six Paths, and there is no limit to the number of DynamicViews you can create. So theoretically, you could have data from dozens of Paths available on your watch! Practically, you'll probably want only the most important Paths to be on your DynamicView screens, but that's entirely up to you.

![image](https://user-images.githubusercontent.com/15186790/140789761-97ee13f6-3feb-4521-9606-0127ba0ffb9a.png)
*An example of a DynamicView screen - this one shows data about the watch itself, but yours will probably show engine data, or wind data, or environmental data, etc.*

DynamicViews are defined with JSON, and it would not be practical to create and edit JSON files on the watch itself, so TWatchSKDesigner was created. [Click here](https://github.com/JohnySeven/TWatchSKDesigner#twatchskdesigner) to read all about that program: how to install it, how to use it to design DynamicViews, and how to get those DynamicViews onto TWatchSK.

BAS: To Do
- Make a short video showing the main functions of the Watch, including some DynamicViews and the display of an SK Notification.
