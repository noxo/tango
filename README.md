This repository is providing source codes of Tango projects I created. **Before using anything read please [the licence](https://github.com/lvonasek/tango/blob/master/LICENSE)**.

## Apps for 3D scanning:
The main project of this repository is [Open Constructor](https://github.com/lvonasek/tango/wiki/Open-Constructor) - the 3D scanner for Tango devices:
* Lenovo Phab 2 Pro
* Asus Zenfone AR (supported as long as this device has Android 7.0 only - Google is going to remove Tango from this device)
* Google Yellowstone tablet (supported as long as this device has no update from 06/2017 - Google removed Tango from this device)

Download this app [from Google Play](https://play.google.com/store/apps/details?id=com.lvonasek.openconstructor)

This app was also ported on ARcore as [3D Scanner for ARcore](https://github.com/lvonasek/tango/wiki/3D-Scanner-for-ARcore) but this version **is not opensource**.

Anyway you can get this app [from Google Play](https://play.google.com/store/apps/details?id=com.lvonasek.arcore3dscanner).

## Apps for rendering 3D models
This apps are not using Tango but they are showing models created by Tango. On Android you can use Daydream viewer to show the content in virtual reality. You can download it [from Google Play](https://play.google.com/store/apps/details?id=com.lvonasek.daydreamOBJ).

Additionaly to this app I developed GLUT viewer which is basically doing the same like Daydream viewer but it is not using virtual reality and it is for Linux. This app is used for debugging purposes.

## Utils

For testing purposes I created the vizualisation tool for showing Tango depth data. The tool can be also used as night vision. You can get this app [from Google Play](https://play.google.com/store/apps/details?id=com.lvonasek.nightvision).

As I created multiple apps I wanted to reduce amount of copying the same code. For that there is common folder which has some kind of SDK function.
