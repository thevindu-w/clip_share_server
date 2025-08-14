# ClipShare - Server

### Share Clipboard and Files. Copy on one device. Paste on another device.

![Build and Test](https://github.com/thevindu-w/clip_share_server/actions/workflows/build-test.yml/badge.svg?branch=master)
[![Last commit](https://img.shields.io/github/last-commit/thevindu-w/clip_share_server.svg?color=yellow)](https://github.com/thevindu-w/clip_share_server/commits/master)
[![License](https://img.shields.io/github/license/thevindu-w/clip_share_server.svg?color=blue)](https://www.gnu.org/licenses/gpl-3.0.en.html#license-text)

[![Latest release](https://img.shields.io/github/v/release/thevindu-w/clip_share_server?color=blue)](https://github.com/thevindu-w/clip_share_server/releases)
[![Downloads](https://img.shields.io/github/downloads/thevindu-w/clip_share_server/total?color=purple)](https://github.com/thevindu-w/clip_share_server/releases)
[![Stars](https://img.shields.io/github/stars/thevindu-w/clip_share_server)](https://github.com/thevindu-w/clip_share_server/stargazers)

<br>

ClipShare is a lightweight and cross-platform tool for clipboard sharing. ClipShare enables copying text, files, and images on one device and pasting on another. ClipShare is simple and easy to use while being highly configurable. The ClipShare server runs in the background on your desktop/laptop. Clients can connect to the server and share copied text, files, and images.

## Download

### Server

<table>
<tr>
<th style="text-align:center">Desktop</th>
</tr>
<tr>
<td align="center">
<a href="https://github.com/thevindu-w/clip_share_server/releases"><img src="https://raw.githubusercontent.com/thevindu-w/clip_share_client/master/fastlane/metadata/android/en-US/images/icon.png" alt="Get it on GitHub" width="100"/></a><br>
Download the server from <a href="https://github.com/thevindu-w/clip_share_server/releases">Releases</a>.
</td>
</tr>
</table>

### Client

<table>
<tr>
<th style="text-align:center">Android</th>
<th style="text-align:center">Desktop</th>
</tr>
<tr>
<td align="center">
<a href="https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare"><img src="https://gitlab.com/IzzyOnDroid/repo/-/raw/master/assets/IzzyOnDroid.png" alt="Get it on IzzyOnDroid" width="250"/></a><br>
Download the Android client app
from <a href="https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare">apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare</a>.<br>
or from <a href="https://github.com/thevindu-w/clip_share_client/releases">GitHub Releases</a>.
</td>
<td align="center">
<a href="https://github.com/thevindu-w/clip_share_desktop/releases"><img src="https://raw.githubusercontent.com/thevindu-w/clip_share_client/master/fastlane/metadata/android/en-US/images/icon.png" alt="Get it on GitHub" width="100"/></a><br>
Download the desktop client from <a href="https://github.com/thevindu-w/clip_share_desktop/releases">Releases</a>.
</td>
</tr>
</table>

<br>

## Table of Contents

- [How to Use](#how-to-use)
  - [Install dependencies](#install-dependencies)
  - [Run the server](#run-the-server)
  - [Allow through firewall](#allow-through-firewall)
  - [Connect the client application](#connect-the-client-application)
  - [Installation](#installation) (optional)
    - [Online installers](#online-installers)
    - [Standalone installers](#standalone-installers)
  - [Create SSL/TLS certificates and key files](#create-ssltls-certificates-and-key-files) (optional)
  - [Command line options](#command-line-options)
  - [Configuration](#configuration)
- [Build from Source](#build-from-source)
  - [Build tools](#build-tools)
  - [Dependencies](#dependencies)
  - [Compiling](#compiling)

<br>

## How to Use

### Install dependencies

**Note:** This section is required only for macOS.

ClipShare needs the following libraries to run:

* [libunistring](https://formulae.brew.sh/formula/libunistring)
* [libssl](https://formulae.brew.sh/formula/openssl@3)
* [libpng](https://formulae.brew.sh/formula/libpng)

These libraries can be installed using [Homebrew](https://brew.sh) with the following command:
```bash
brew install libunistring openssl@3 libpng
```

<br>

### Run the server

- You can run the server from a terminal or the GUI (if the file manager supports executing programs by double-clicking on it).
- On macOS, if you run the server from the GUI (by double-clicking it on Finder), it may open a terminal window. But you can close this window. The server will continue running in the background.
- When the server starts, it will not display any visible window. Instead, it will run in the background.
- On Linux or macOS, if you start the program from the terminal, it should return immediately unless the `-D` flag (no-daemonize) is used. The server will continue to run in the background.
- On Windows, it will show a tray icon unless disabled from the configuration file. You can click on it to stop the server.
- If something goes wrong, it will create a `server_err.log` file containing what went wrong.

<br>

### Allow through firewall

  This server listens on the following ports (unless different ports are assigned from the configuration),

* `4337` / `TCP` &nbsp; - &nbsp; For application traffic (not encrypted)
* `4337` / `UDP` &nbsp; - &nbsp; For network scanning using UDP broadcasts
* `4338` / `TCP` &nbsp; - &nbsp; For application traffic over TLS (encrypted)
* `4339` / `TCP` &nbsp; - &nbsp; For the web server (only if the web server is available)

You may need to allow incoming connections to the above ports for the client to connect to the server.

Note that all TCP ports are for unicast, while `4337/udp` is used to receive broadcasts. Therefore, the firewall rule that allows `4337/udp` should have the **broadcast** address of the interface as the destination address.

<br>

### Connect the client application

You can find an Android client app in [releases](https://github.com/thevindu-w/clip_share_client/releases). You can also get it from [apt.izzysoft.de](https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare/). The source of the Android client app is available at [github.com/thevindu-w/clip_share_client](https://github.com/thevindu-w/clip_share_client). Or you may develop a client app according to the protocol specification described in the `docs/`.<br>
- The client and the server devices should be on the same network. You can do that by connecting both devices to the same Wi-Fi network. It is also possible to use one of the devices as a Wi-Fi hotspot and connect the other device to that hotspot.
- If the client supports network scanning, it can automatically find the server in the network. Otherwise, enter the server's IP address to the client.
- Now, the client can share clipboard data and files and get images from the server.<br>
Note that the server should allow the client through the firewall, as mentioned in the above section.

<br>

### Installation

**Note:** This section is optional if you prefer manually starting the server over automatically starting on login/reboot.

To install the server to run on startup, use the corresponding installer script for your platform.

#### Online installers

Online installer scripts are attached with the releases. They will download the corresponding version during the installation. Therefore, they do not require having the compiled binaries, along with the installer, to run it. However, they require internet access to download the binaries from GitHub during the installation. Run the interactive script and follow the instructions to install ClipShare.

<details>
  <summary>Linux and macOS</summary>

1. Open a terminal in the directory where the `install-linux-mac.sh` installer script is downloaded.
1. Run the install script as shown below, and follow the instructions of it.
```bash
chmod +x install-linux-mac.sh
./install-linux-mac.sh
```
</details>

<details>
  <summary>Windows</summary>

1. Download the `install-windows.bat` file from [release assets](https://github.com/thevindu-w/clip_share_server/releases/latest).
1. Double-click on the `install-windows.bat` installer script to run it. It will open a Command Prompt window. Follow the instructions on it to install ClipShare. (If double-clicking did not run the installer, right-click on it and select Run)
</details>
<br>

#### Standalone installers

Standalone installer scripts are available in the archives (`zip` for Windows and macOS, and `tar.gz` for Linux) attached to releases on GitHub. They are also available in the [helper_tools/](https://github.com/thevindu-w/clip_share_server/tree/master/helper_tools) directory. Standalone installers can run without an internet connection. However, you must have the `clip_share` (or `clip_share.exe` on Windows) executable in the current working directory to run the installer. If you download the archive from releases and extract it, you will have the executable, along with the installer script. Run the interactive script and follow the instructions to install ClipShare.

**Note:** These installers do NOT need admin or superuser privileges to run. Therefore, do not run them with elevated privileges.

<details>
  <summary>Linux and macOS</summary>

1. Open a terminal in the directory where the `clip_share` executable is available (the executable name may have suffixes like `_GLIBC*` on Linux or `arm64` or `x86_64` on macOS).
1. Run the install script as shown below, and follow the instructions of it.
```bash
# on Linux
chmod +x install-linux.sh
./install-linux.sh
```
```bash
# on macOS
chmod +x install-mac.sh
./install-mac.sh
```
</details>

<details>
  <summary>Windows</summary>

1. Place the `install-windows.bat` file and the `clip_share.exe` executable in the same folder. (the executable name may have suffixes)
1. Double-click on the `install-windows.bat` installer script to run it. It will open a Command Prompt window. Follow the instructions on it to install ClipShare. (If double-clicking did not run the installer, right-click on it and select Run)
</details>

<br>

### Create SSL/TLS certificates and key files

**Note:** This section is optional if you do not need the TLS encrypted mode and the web mode.

The following files should be created, and their paths should be specified in the configuration file `clipshare.conf`. You may use different file names and paths to store the keys and certificates.

* `server.pfx` &ensp; - &nbsp; SSL/TLS key and certificate file of the server
* `ca.crt` &emsp;&emsp;&ensp; - &nbsp; SSL/TLS certificate of the CA, which signed both the server's and the client's SSL/TLS certificates

You may use the helper scripts `keygen.sh` or `keygen.ps1` in the [helper_tools/](https://github.com/thevindu-w/clip_share_server/tree/master/helper_tools) directory to generate TLS keys and certificates for both the server and the client. Linux and macOS users can use `keygen.sh`. Windows users can use `keygen.ps1` PowerShell script that doesn't require OpenSSL. Windows users may also use `keygen.sh` if running in a Bash shell (ex: Git Bash). However, this requires OpenSSL version 3.x or later.

<details>
  <summary>Linux and macOS (Using OpenSSL)</summary>
  Keep the `clipshare.ext` file in the same directory as the `keygen.sh` script when running the script.

  ```bash
  # If you download/clone the repository and run the script from the repository root,
  helper_tools/keygen.sh
  ```
  ```bash
  # If you download the script separately and run the script from within the same directory,
  chmod +x keygen.sh
  ./keygen.sh
  ```
  Refer to the [OpenSSL manual](https://www.openssl.org/docs/manmaster/man1/openssl.html) for more information on generating keys.

</details>

<details>
  <summary>Windows (Using PowerShell)</summary>

  ```powershell
  # If you download/clone the repository and run the script from the repository root,
  powershell -ExecutionPolicy bypass -File helper_tools\keygen.ps1
  ```
  ```powershell
  # If you download the script separately and run the script from within the same directory,
  powershell -ExecutionPolicy bypass -File keygen.ps1
  ```
</details>

If you use this script, the server's common name will be `clipshare_server`, and the client's common name will be `clipshare_client`. You may change those names in the script you used.


<br>

### Command line options

`clip_share` accepts the following command line options.
<details>
  <summary>Command line options</summary>

```
./clip_share [-h] [-s] [-r] [-R] [-d] [-D]

  -h   Help         - Display command usage and exit.
                      This takes precedence over other options.

  -v   Version      - Display application version and exit.
                      This takes precedence over other options.

  -s   Stop         - Stop all instances of the server if any.
                      This takes precedence over -r and -R.

  -r   Restart      - Stop other instances of the server if any,
                      and restart the server. This option takes
                      precedence over the restart value in the
                      configuration file.

  -R   No-Restart   - Start the server without restarting. This
                      option takes precedence over the restart
                      value in the configuration file.

  -d   Daemonize    - Exit the main process after creating child
                      processes. This option is effective only on
                      Linux and macOS. (default)

  -D   No-Daemonize - Do not exit the main process after creating
                      child processes. This option is effective
                      only on Linux and macOS.
```
</details>

### Configuration

The ClipShare server can be configured using a configuration file. The configuration file should be named `clipshare.conf`.
The server searches for the configuration file in the following paths in the same order until it finds one.
1. Current working directory where the server was started.
1. `$XDG_CONFIG_HOME` directory if the variable is set to an existing directory (on Linux and macOS only).
1. `~/.config/` directory if the directory exists (on Linux and macOS only).
1. Current user's home directory (also called user profile directory on Windows).

If it can't find a configuration file in any of the above directories, it will use the default values specified in the table below.

To customize the server, create a file named &nbsp; `clipshare.conf` &nbsp; in any of the directories mentioned above and add the following lines to that configuration file. You may omit some lines to keep the default values.
<details>
  <summary>Sample configuration file</summary>

```properties
app_port=4337
app_port_secure=4338
udp_port=4337
web_port=4339
insecure_mode_enabled=true
secure_mode_enabled=true
udp_server_enabled=true
web_mode_enabled=false
server_cert=cert_keys/server.pfx
ca_cert=cert_keys/ca.crt
allowed_clients=allowed_clients.txt
working_dir=./path/to/work_dir
bind_address=0.0.0.0
bind_address_udp=0.0.0.0
restart=true
max_text_length=4194304
max_file_size=68719476736
display=1
client_selects_display=false
cut_sent_files=false
min_proto_version=1
max_proto_version=3

# Windows and macOS only
tray_icon=true
```
</details>

Note that all the lines in the configuration file are optional. You may omit some lines if they need to get their default values.
<br>
<details>
  <summary>Configuration options</summary>

| Property | Description | Accepted values | Default |
|  :----:  | :--------   | :------------   |  :---:  |
| `insecure_mode_enabled` | Whether or not the application listens for unencrypted connections. The values `true` or `1` will enable it, while `false` or `0` will disable it. | `true`, `false`, `1`, `0` (Case insensitive) | `true` |
| `secure_mode_enabled` | Whether or not the application listens for TLS-encrypted connections. The values `true` or `1` will enable it, while `false` or `0` will disable it. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `udp_server_enabled` | Whether or not the application listens for UDP scanning requests. The values `true` or `1` will enable it, while `false` or `0` will disable it. | `true`, `false`, `1`, `0` (Case insensitive) | `true` |
| `web_mode_enabled` | Whether or not the application listens for TLS-encrypted connections from web clients if the web mode is available. The values `true` or `1` will enable it, while `false` or `0` will disable it. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `app_port` | The port on which the application listens for unencrypted TCP connections. (Values below 1024 may require superuser/admin privileges) | Any valid, unused TCP port number (1 - 65535) | `4337` |
| `udp_port` | The port on which the application listens for UDP broadcasts of network scanning. (Values below 1024 may require superuser/admin privileges) | Any valid, unused UDP port number (1 - 65535) | `4337` |
| `app_port_secure` | The TCP port on which the application listens for TLS-encrypted connections. (Values below 1024 may require superuser/admin privileges) | Any valid, unused TCP port number (1 - 65535) | `4338` |
| `web_port` | The TCP port on which the application listens for TLS-encrypted connections for web clients. This setting is used only if web mode is available. (Values below 1024 may require superuser/admin privileges) | Any valid, unused TCP port number (1 - 65535) | `4339` |
| `server_cert` | The TLS key and certificate store file of the server. If this is not specified, secure mode (and web mode if available) will be disabled. | Absolute or relative path to the server's TLS certificate store PKCS#12 file | \<Unspecified\> |
| `ca_cert` | The TLS certificate file of the CA that signed the TLS certificate of the server. If this is not specified, secure mode (and web mode if available) will be disabled. | Absolute or relative path to the TLS certificate PEM file of the CA | \<Unspecified\> |
| `allowed_clients` | The text file containing a list of allowed clients (Common Name of client certificate), one name per line. If this is not specified, secure mode (and web mode if available) will be disabled. | Absolute or relative path to the allowed-clients file | \<Unspecified\> |
| `bind_address` | The address of the interface to which the application should bind when listening for connections. It will listen on all interfaces if this is set to `0.0.0.0` | IP address of an interface or wildcard address. IPv4 dot-decimal notation (ex: `192.168.37.5`) or `0.0.0.0`, or IPv6 hexadecimal notation (ex: `fc00::abcd:12`) or `::` | `0.0.0.0` |
| `bind_address_udp` | The IP address to which the application should bind when listening for UDP scanning requests. It will listen on all addresses if this is set to `0.0.0.0` | IP address of an interface or wildcard address. IPv4 dot-decimal notation (ex: `192.168.37.5`) or `0.0.0.0`, or IPv6 hexadecimal notation (ex: `fc00::abcd:12`) or `::` | `0.0.0.0` |
| `restart` | Whether the application should start or restart by default. The values `true` or `1` will make the server restart by default, while `false` or `0` will make it just start without stopping any running instances of the server. | `true`, `false`, `1`, `0` (Case insensitive) | `true` |
| `working_dir` | The working directory where the application should run. All the files, that are sent from a client, will be saved in this directory. It will follow symlinks if this is a path to a symlink. The user running this application should have write access to the directory | Absolute or relative path to an existing directory | `.` (i.e. Current directory) |
| `max_text_length` | The maximum length of text that can be transferred. This is the number of bytes of the text encoded in UTF-8. | Any integer between 1 and 4294967295 (nearly 4 GiB) inclusive. Suffixes K, M, and G (case insensitive) denote x10<sup>3</sup>, x10<sup>6</sup>, and x10<sup>9</sup>, respectively. | 4194304 (i.e. 4 MiB) |
| `max_file_size` | The maximum size of a single file in bytes that can be transferred. | Any integer between 1 and 9223372036854775807 (nearly 8 EiB) inclusive. Suffixes K, M, G, and T (case insensitive) denote x10<sup>3</sup>, x10<sup>6</sup>, x10<sup>9</sup>, and x10<sup>12</sup>, respectively. | 68719476736 (i.e. 64 GiB) |
| `display` | The display that should be used for screenshots. | Display number (1 - 65535) | `1` |
| `cut_sent_files` | Whether to automatically cut the files into the clipboard on the _Send Files_ method. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `client_selects_display` | Whether the client can override the default/configured display for screenshots in protocol version 3. The values `true` or `1` will allow overriding the default, while `false` or `0` will force using the default/configured display. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `min_proto_version` | The minimum protocol version the server should accept from a client after negotiation. | Any protocol version number greater than or equal to the minimum protocol version the server has implemented. (ex: `2`) | The minimum protocol version the server has implemented |
| `max_proto_version` | The maximum protocol version the server should accept from a client after negotiation. | Any protocol version number less than or equal to the maximum protocol version the server has implemented. (ex: `3`) | The maximum protocol version the server has implemented |
| `method_get_text_enabled`<br>`method_send_text_enabled`<br>`method_get_files_enabled`<br>`method_send_files_enabled`<br>`method_get_image_enabled`<br>`method_get_copied_image_enabled`<br>`method_get_screenshot_enabled`<br>`method_info_enabled` | These configuration keys map to methods in ClipShare. They separately define whether the corresponding method is enabled or not. The values `true` or `1` will allow clients to use the method, while `false` or `0` will disable the method. | `true`, `false`, `1`, `0` (Case insensitive) | `true` |
| `tray_icon` | Whether the application should display a system tray icon (menu icon on macOS). This option is available only on Windows and macOS. The values `true` or `1` will display the icon, while `false` or `0` will prevent displaying the icon. | `true`, `false`, `1`, `0` (Case insensitive) | `true` |

<br>

You may change these values. But it is recommended to keep the port numbers unchanged. If the port numbers are changed, client application configurations may also need to be changed as appropriate to connect to the server.
<br>
</details>

If you changed the configuration file, you must restart the server to apply the changes.
<br>
<br>

## Build from Source

**Note:** If you prefer using the pre-built binaries from [Releases](https://github.com/thevindu-w/clip_share_server/releases), you can ignore this section and follow the instructions in the [How to Use](#how-to-use) section.

### Build tools

  Compiling ClipShare needs the following tools,

* gcc (or Clang for Windows x64)
* make

<details>
  <summary>Linux</summary>

  On Linux, these tools can be installed with the following command:

* On Debian-based or Ubuntu-based distros,
  ```bash
  sudo apt-get install gcc make
  ```

* On Redhat-based or Fedora-based distros,
  ```bash
  sudo yum install gcc make
  ```

* On Arch-based distros,
  ```bash
  sudo pacman -S gcc make
  ```
</details>

<details>
  <summary>Windows</summary>

  On Windows, these tools can be installed with [MSYS2](https://www.msys2.org/).<br>
  In an MSYS2 environment, these tools can be installed using pacman with the following command:
  ```bash
  pacman -S mingw-w64-clang-x86_64-clang make
  ```
</details>

<details>
  <summary>macOS</summary>

  On macOS, these tools are installed with Xcode Command Line Tools.
</details>

<br>

### Dependencies

<details>
  <summary>Linux</summary>

  The following development libraries are required.

* libc
* libx11
* libxmu
* libxcb-randr
* libpng
* libssl
* libunistring

  They can be installed with the following command:

* On Debian-based or Ubuntu-based distros,
  ```bash
  sudo apt-get install libc6-dev libx11-dev libxmu-dev libxcb-randr0-dev libpng-dev libssl-dev libunistring-dev
  ```

* On Redhat-based or Fedora-based distros,
  ```bash
  sudo yum install glibc-devel libX11-devel libXmu-devel libpng-devel openssl-devel libunistring-devel
  ```

* On Arch-based distros,
  ```bash
  sudo pacman -S libx11 libxmu libpng openssl libunistring
  ```

  glibc should already be available on Arch distros. But you may need to upgrade it with the following command. (You need to do this only if the build fails)

  ```bash
  sudo pacman -S glibc
  ```

(You may refer to docker/Dockerfile to see how to install the dependencies on various Linux distros)
</details>

<details>
  <summary>Windows</summary>

  The following development libraries are required.

* [libz](https://packages.msys2.org/package/mingw-w64-x86_64-libzip?repo=mingw64)
* [libpng16](https://packages.msys2.org/package/mingw-w64-x86_64-libpng?repo=mingw64)
* [libssl](https://packages.msys2.org/package/mingw-w64-x86_64-openssl?repo=mingw64) (provided by OpenSSL)
* [libunistring](https://packages.msys2.org/package/mingw-w64-x86_64-libunistring?repo=mingw64)

In an [MSYS2](https://www.msys2.org/) environment, these tools can be installed using pacman with the following commands:
```bash
pacman -S mingw-w64-clang-x86_64-libpng mingw-w64-clang-x86_64-libunistring
pacman -S mingw-w64-clang-x86_64-openssl # See the note below
```

**Note:** However, to avoid DLL loading issues, it is recommended to compile openssl from its source with the following configuration and use that instead of the `mingw-w64-clang-x86_64-openssl` packaged version.
```bash
OPENSSL_PATH='<Use a suitable existing empty directory path here>'
CC=clang ./Configure zlib no-zlib-dynamic no-brotli no-zstd no-shared no-tests no-docs --prefix="$OPENSSL_PATH" mingw64
make
make install
```
To use it, set the following environment variables in the shell used to build ClipShare.
```bash
export CPATH="${OPENSSL_PATH}/include"
export LIBRARY_PATH="${OPENSSL_PATH}/lib64"
```

Additionally, [PatchPE](https://github.com/datadiode/PatchPE) should be available, and the PATH environment variable should be set accordingly. Download PatchPE from [github.com/datadiode/PatchPE/releases/download/2.04/PatchPE.exe](https://github.com/datadiode/PatchPE/releases/download/2.04/PatchPE.exe).
</details>

<details>
  <summary>macOS</summary>

The following development libraries are required.

* [openssl](https://formulae.brew.sh/formula/openssl@3)
* [libpng](https://formulae.brew.sh/formula/libpng)
* [libunistring](https://formulae.brew.sh/formula/libunistring)

These libraries can be installed using [Homebrew](https://brew.sh) with the following command:
```bash
brew install openssl@3 libpng libunistring
```
</details>

<br>

### Compiling

1. Open a terminal / command prompt / Powershell in the project directory

    This can be done using the GUI or the `cd` command.

1. Run the following command to make the executable file

    ```bash
    make
    ```
    This will generate the executable named clip_share (or clip_share.exe on Windows).
