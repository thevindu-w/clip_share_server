This is Version <VERSION> with support for protocol versions 1, 2, and 3.

**Notes:**
- To use ClipShare, you will need a client app. An Android client app is available at [apt.izzysoft.de](https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare/). (Source: [github.com/thevindu-w/clip_share_client](https://github.com/thevindu-w/clip_share_client))
- The `clipshare.conf` file in assets is a sample. You may need to modify it.
- Windows version is tested on Windows 7 and later. It has 32-bit (`x86`) and 64-bit (`x86_64`) versions. You can select the one that is suitable for your OS. If you use the installer script, it will automatically select the suitable version.
- There is a `no_ssl` variant for Windows. It is smaller in size but lacks SSL/TLS support.
- There are multiple Linux server versions included in assets in the `<FILE_LINUX_AMD64>` archive. They are compiled for various GLIBC versions and libssl versions. You can select the one that is compatible with your system. If you use the installer script, it will automatically select the suitable version. If none of them are working on your system, you need to compile it from the source. The compiling procedure is described in [README.md#build-from-source](https://github.com/thevindu-w/clip_share_server#build-from-source).
- There are two versions for macOS included in assets in the `<FILE_MACOS>` archive. They are compiled for Intel-based Mac and Mac computers with Apple silicon. You can select the one that is compatible with your Mac computer. If you use the installer script, it will automatically select the suitable version.
  - For Mac computers with Apple silicon, use `clip_share-arm64`.
  - For Intel-based Mac, use `clip_share-x86_64`.
- You need to allow the app through the firewall. Refer to [README.md#allow-through-firewall](https://github.com/thevindu-w/clip_share_server#allow-through-firewall) and [README.md#how-to-use](https://github.com/thevindu-w/clip_share_server#how-to-use) sections for more details.
- Optionally, you may install `clip_share` to run on startup. Refer to [README.md#installation](https://github.com/thevindu-w/clip_share_server#installation) for more details.
- The installer scripts attached here are online installers. They will automatically download the correct binaries for this version and run the offline installer in it. Alternatively, you may manually download the binary archive for your operating system and use the offline installer in it.
- Installers do not need admin or superuser privileges to run.

**Changes:**
- Fix DLL search order hijacking vulnerability in Windows x64 build.
- Code quality improvements.
