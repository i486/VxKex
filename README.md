Introduction
============

VxKex is a set of API extensions for Windows 7 that allow some Windows 8,
8.1 and 10-exclusive applications to run on Windows 7.

To download and install, see the [releases page](https://github.com/vxiiduu/VxKex/releases).

After installation, usage is easy: just right click on a program (.exe or .msi),
open the Properties dialog, and select the VxKex tab. Then, check the check box
which says "Enable VxKex for this program", and try to run the program.

![VxKex configuration GUI](/example-screenshot.png)

Some programs require additional configuration. There's a file called **"Application
Compatibility List.docx"** inside the VxKex installation folder (which is C:\Program
Files\VxKex by default) which details these steps, but for the most part, all
configuration is self-explanatory.

If you are a developer, source code is provided as a 7z file on the releases page.

FAQ
===

**Q: Does it work for games???**

**A:** At the moment, VxKex is not intended for games, so you will probably have limited
success. I hope to address this shortcoming in a future release.

**Q: What applications are supported?**

**A:** The list of compatible applications includes, but is not limited to:
- Bespoke Synth
- Blender
- Blockbench
- Calibre
- Chromium (including Ungoogled Chromium)
- Citra
- Commander Wars
- Cygwin
- Dasel
- Discord Canary
- ElectronMail
- Firefox
- GIMP (2.99.18)
- GitHub Desktop
- HandBrake
- Kodi
- MKVToolNix
- MongoDB
- MPC-Qt
- MPV
- MPV.NET
- Opera
- osu!lazer
- Python
- qBittorrent
- QMMP
- Qt Creator
- Rufus
- Steel Bank Common Lisp
- Spotify
- Steinberg SpectraLayers
- TeamTalk
- VSCode and VSCodium
- WinDbg (classic from Windows 11 SDK, and preview)
- Yuzu (gameplay was not tested)
- Zig

See the **Application Compatibility List.docx** file, which is installed together
with VxKex, for more information.

The majority of Qt6 applications will work, and many Electron applications will
work as well.

**Q: Does VxKex modify system files? Will it make my system unstable?**

**A:** VxKex does not modify any system files. Its effect on the whole system is
extremely minimal. No background services are used, no global hooks are
installed, and the shell extensions and DLLs that are loaded have minimal
impact and can be disabled if needed. You can rest assured that your Windows 7
will remain as stable as it always is.

**Q: Do I need to have specific updates installed?**

**A:** VxKex is only designed to work with Service Pack 1 installed. Users of
Windows 7 RTM can try to use it, but I don't know if it will install or work.
Many programs require KB2533623 and KB2670838 in order to run. It is a good
idea to install those two updates.

**Q: If I have ESUs (Extended Security Updates) installed, can I use VxKex?**

**A:** Yes. There is no problem with ESUs.

**Q: Do console applications work with VxKex?**

**A:** Yes. After you have enabled VxKex for a program you can use it through the
command prompt as normal.

**Q: Can I use this with Windows 8 or 8.1?**

**A:** VxKex is designed for use only with Windows 7. If you use Windows 8 or 8.1,
it's unlikely that VxKex will do anything useful, but you're free to install it
anyway and see what happens.

**Q: How does VxKex work?**

**A:** VxKex works by loading a DLL into each program where VxKex is enabled. This
is accomplished through using the IFEO (Image File Execution Options) registry key.

Specifically, the "VerifierDlls" value is set to point to a VxKex DLL. This DLL then
loads into the process.

API extension is accomplished by editing the program's DLL import table so that
instead of importing from Windows 8/8.1/10 DLLs, it imports to VxKex DLLs instead.
These VxKex DLLs contain implementations of Windows API functions which were introduced
in newer versions of Windows.

Donations
=========

If you would like to support development, consider making a donation.

- https://paypal.me/vxiiduu
