This is Version <VERSION> with support for protocol versions 1, 2, and 3.

**Notes:**
- You will need a client app to use ClipShare. You can find a client app for Android at [apt.izzysoft.de](https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare/). Its source is available at [github.com/thevindu-w/clip_share_client](https://github.com/thevindu-w/clip_share_client).
- The `clipshare.conf` file in assets is a sample. You may need to modify it.
- There are multiple Linux server versions included in assets in the `<FILE_LINUX>` archive. They are compiled for some GLIBC versions (2.27, 2.31, 2.35, and 2.39) with some libssl versions (1.1 and 3). You can select the one that is compatible with your Linux system. If you use the installer script, it will automatically select the suitable version. If none of them are working on your system, you need to compile it from the source for your distribution. The compiling procedure is described in [README.md#build-from-source](https://github.com/thevindu-w/clip_share_server#build-from-source).
  - For Ubuntu 24 (Noble) based distros, the version with GLIBC 2.39 and libssl-3 should work.
  - For Ubuntu 22 (Jammy) based distros, the version with GLIBC 2.35 and libssl-3 should work.
  - For Ubuntu 20 (Focal) based distros, the version with GLIBC 2.31 and libssl-1 should work.
  - For Ubuntu 18 (Bionic) based distros, the version with GLIBC 2.27 and libssl-1 should work.
  - For the latest versions of Fedora or Arch based distros, the version with GLIBC 2.39 or later should work.
- Windows version is tested only on Windows 10 and later. It might fail on older versions.
- There are two macOS server versions included in assets in the `<FILE_MACOS>` archive. They are compiled for Intel-based Mac and Mac computers with Apple silicon. You can select the one that is compatible with your Mac computer. If you use the installer script, it will automatically select the suitable version.
  - For Mac computers with Apple silicon, use `clip_share-arm64`.
  - For Intel-based Mac, use `clip_share-x86_64`.
- You need to allow the app through the firewall. Refer to [README.md#allow-through-firewall](https://github.com/thevindu-w/clip_share_server#allow-through-firewall) and [README.md#how-to-use](https://github.com/thevindu-w/clip_share_server#how-to-use) sections for more details.
- Optionally, you may install `clip_share` to run on startup. Refer to [README.md#installation](https://github.com/thevindu-w/clip_share_server#installation) for more details.
- The installer scripts attached to the assets are online installers. They will download the correct binaries for this version automatically, and run the offline installer in it. Alternatively, you may manually download the binary archive for your operating system and use the offline installer in it.

**Changes:**
- Bug fixes
  - Fix possible failure when sending large files above 2 GiB
  - Fix possible memory leak
- Add online installers to release assets
- Fix possible compilation issues on some platforms