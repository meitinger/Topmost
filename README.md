Topmost
=======


Description
-----------
This very small (< 200 LoC) utility allows the user to select any window as a
topmost window, commonly known as "always on top". It does so by inserting an
entry into each window's system menu (the ALT+SPACE one). The entry's text is
localized by using the task manager as a resource. (Which certainly isn't best
practice but has been working fine since WinXP.)

Build and Usage
---------------
There are two VS projects: Hook and Topmost. The latter is the application that
initializes the hook, the former is the actual hook DLL that gets injected into
every process. The release build is set to produce a multithreaded static DLL,
so there are no external dependences which should make deployment fairly easy.

After building with Visual Studio, you end up with a 32bit and 64bit version.
Each will add the entry to x86 and x64 applications respectively, so you have
to start both on a 64bit version of Windows.

The application will run for as long as the user is logged in and doesn't
display any messages, so it's easy to just make it auto-start during logon.
