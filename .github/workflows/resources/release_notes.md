This is Version 3.2.0 with support for protocol versions 1, 2, and 3.

**Notes:**
- You will need a client app to use ClipShare. You can find a client app for Android at [apt.izzysoft.de](https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare/). Its source is available at [github.com/thevindu-w/clip_share_client](https://github.com/thevindu-w/clip_share_client).
- The `clipshare.conf` file in assets is a sample. You may need to modify it.
- There are multiple Linux server versions included in assets in the `clip_share_server-3.2.0-linux_x86_64.tar.gz` archive. They are compiled for some GLIBC versions (2.27, 2.31, 2.35, and 2.39) with some libssl versions (1.1 and 3). You can select the one compatible with your Linux system if there is any. They might not work for other GLIBC versions and libssl versions. If none of them are working on your system, you need to compile it from the source for your distribution. The compiling procedure is described in [README.md#build-from-source](https://github.com/thevindu-w/clip_share_server#build-from-source).
  - For Ubuntu 24 (Noble) based distros, the version with GLIBC 2.39 and libssl-3 should work.
  - For Ubuntu 22 (Jammy) based distros, the version with GLIBC 2.35 and libssl-3 should work.
  - For Ubuntu 20 (Focal) based distros, the version with GLIBC 2.31 and libssl-1 should work.
  - For Ubuntu 18 (Bionic) based distros, the version with GLIBC 2.27 and libssl-1 should work.
  - For the latest versions of Fedora or Arch based distros, the version with GLIBC 2.39 or later should work.
- Windows version is tested only on Windows 10 and later. It might fail on older versions.
- There are two macOS server versions included in assets in the `clip_share_server-3.2.0-macos.zip` archive. They are compiled for Intel-based Mac and Mac computers with Apple silicon. You can select the one compatible with your Mac computer.
  - For Mac computers with Apple silicon, use `clip_share-arm64`.
  - For Intel-based Mac, use `clip_share-x86_64`.
- You need to allow the app through the firewall. Refer to [README.md#allow-through-firewall](https://github.com/thevindu-w/clip_share_server#allow-through-firewall) and [README.md#how-to-use](https://github.com/thevindu-w/clip_share_server#how-to-use) sections for more details.
- Optionally, you may install `clip_share` to run on startup. Refer to [README.md#installation](https://github.com/thevindu-w/clip_share_server#installation) for more details.

**Changes:**
- Option to cut the sent files to the clipboard is now available on Windows.