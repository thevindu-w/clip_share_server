This is Version <VERSION> with support for protocol versions 1, 2, and 3.

**Notes:**
- To use ClipShare, you will need a client app. An Android client app can be found at [apt.izzysoft.de](https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare/). Its source is available at [github.com/thevindu-w/clip_share_client](https://github.com/thevindu-w/clip_share_client).
- The `clipshare.conf` file in assets is a sample. You may need to modify it.
- There are multiple Linux server versions included in assets in the `<FILE_LINUX>` archive. They are compiled for GLIBC versions (2.27, 2.31, 2.35, and 2.39) and libssl versions (1.1 and 3). You can select the one that is compatible with your Linux system. If you use the installer script, it will automatically select the suitable version. If none of them are working on your system, you need to compile it from the source. The compiling procedure is described in [README.md#build-from-source](https://github.com/thevindu-w/clip_share_server#build-from-source).
- Windows version is tested only on Windows 10 and later. It might fail on older versions.
- There are two versions for macOS included in assets in the `<FILE_MACOS>` archive. They are compiled for Intel-based Mac and Mac computers with Apple silicon. You can select the one that is compatible with your Mac computer. If you use the installer script, it will automatically select the suitable version.
  - For Mac computers with Apple silicon, use `clip_share-arm64`.
  - For Intel-based Mac, use `clip_share-x86_64`.
- You need to allow the app through the firewall. Refer to [README.md#allow-through-firewall](https://github.com/thevindu-w/clip_share_server#allow-through-firewall) and [README.md#how-to-use](https://github.com/thevindu-w/clip_share_server#how-to-use) sections for more details.
- Optionally, you may install `clip_share` to run on startup. Refer to [README.md#installation](https://github.com/thevindu-w/clip_share_server#installation) for more details.
- The installer scripts attached to the assets are online installers. They will automatically download the correct binaries for this version, and run the offline installer in it. Alternatively, you may manually download the binary archive for your operating system and use the offline installer in it.

**Changes:**
- Show version with -v command line option.
- Fix working_dir configuration values containing back-slashes or ending slashes on Windows.
- Enforce minimum TLS version 1.2.
- Code quality improvements.
