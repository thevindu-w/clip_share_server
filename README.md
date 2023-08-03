![Build](https://github.com/thevindu-w/clip_share_server/actions/workflows/build_and_test.yml/badge.svg?branch=master)
![Last commit](https://img.shields.io/github/last-commit/thevindu-w/clip_share_server.svg?color=yellow)
![License](https://img.shields.io/github/license/thevindu-w/clip_share_server.svg?color=blue)

# Clip Share

### Copy on one device. Paste on another device

<br>
This is the server that runs in the background. Cilents can connect to the server and share copied text, files, and images.
<br>
<br>

## Building

### Tools

  This needs the following tools,

* gcc
* make

#### Linux

  On Linux, they can be installed with the following command:

* On Debian or Ubuntu based distros,
  ```bash
  sudo apt-get install gcc make
  ```

* On Redhat or Fedora based distros,
  ```bash
  sudo yum install gcc make
  ```

* On Arch based distros,
  ```bash
  sudo pacman -S gcc make
  ```

#### Windows

  On Windows, these tools can be installed with [MinGW](https://www.mingw-w64.org/).

<br>

### Dependencies

#### Linux

  The following development libraries are required.

* libc
* libx11
* libxmu
* libpng
* libssl

  They can be installed with the following command:

* On Debian or Ubuntu based distros,
  ```bash
  sudo apt-get install libc6-dev libx11-dev libxmu-dev libpng-dev libssl-dev
  ```

* On Redhat or Fedora based distros,
  ```bash
  sudo yum install glibc-devel libX11-devel libXmu-devel libpng-devel openssl-devel
  ```

* On Arch based distros,
  ```bash
  sudo pacman -S libx11 libxmu libpng openssl
  ```

  glibc should already be available on Arch distros. But you may need to upgrade it with the following command. (You need to do this only if the build fails)

  ```bash
  sudo pacman -S glibc
  ```

(You may refer to docker/Dockerfile\* to see how to install the dependencies on various Linux distros)

#### Windows

  The following development libraries are required.

* [libz](https://packages.msys2.org/package/mingw-w64-x86_64-libzip?repo=mingw64)
* [libpng16](https://packages.msys2.org/package/mingw-w64-x86_64-libpng?repo=mingw64)
* [libssl](https://packages.msys2.org/package/mingw-w64-x86_64-openssl?repo=mingw64) (provided by OpenSSL)

<br>

### Compiling

1. Open a terminal / command prompt / Powershell in the project directory

    This can be done using the GUI or the `cd` command.

1. Run the following command to make the executable file

    ```bash
    make
    ```
    This will generate the executable named clip_share (or clip_share.exe on Windows).

    **Note**: The web version is deprecated.<br>
    To compile with the web server enabled, (Currently, this is tested only on Linux)
    ```bash
    make web
    ```
    This will generate the web server enabled executable named clip_share_web.

<br>
<br>

## Allow through firewall

  This server listens on the following ports (unless different ports are assigned from the configuration),

* `4337` / `TCP` &nbsp; - &nbsp; For application traffic (not encrypted)
* `4337` / `UDP` &nbsp; - &nbsp; For network scanning
* `4338` / `TCP` &nbsp; - &nbsp; For application traffic over TLS (encrypted)
* `4339` / `TCP` &nbsp; - &nbsp; For the web server (if the web server is available)

You may need to allow incoming connections to the above ports for the client to connect to the server.

<br>

## Create SSL/TLS certificate and key files

**Note**: This section is optional if you do not need the TLS encrypted mode and the web mode.
<br>
The following files should be created and placed in the `cert_keys/` directory and specified in the configuration file `clipshare.conf`. You may use different file names and paths to store the keys and certificates. Refer [OpenSSL manual](https://www.openssl.org/docs/manmaster/man1/openssl.html) for more information on generating keys.

* `server.key` &ensp; : &nbsp; SSL/TLS key file for the server
* `server.crt` &ensp; : &nbsp; SSL/TLS certificate file of the server
* `ca.crt` &emsp;&emsp;&ensp; : &nbsp; SSL/TLS certificate of the CA which signed the server.crt

<br>
<br>

## How to use
### Run the server

- You can run the server from a terminal or the GUI (if the file manager supports executing programs by double-clicking on it)
- When the server starts, it will not open any window. Instead, it will run in the background.
- On Linux, if you start the program from the terminal, it should return immediately (the server will continue to run in the background).
- If something goes wrong, it will create a `server_err.log` filefile. That file will contain what went wrong.

### Connect the client application

You can find an Android client app in `releases`. Or you may develop a client app according to the protocol specification described in the `docs/`.<br>
- The client and the server devices should be on the same network. You can do that by connecting both devices to the same Wi-Fi network. It is also possible to use one of the devices as a Wi-Fi hotspot and connect the other device to that hotspot.
- If the client supports network scanning, it can easily find the server in the network. Otherwise, enter the server's IPv4 address to the client.
- Now the client can share clipboard data and files and get images from the server.<br>
Note that the server should allow the client through the firewall, as mentioned in the above section.

<br>

## Configuration
Create a file named &nbsp; `clipshare.conf` &nbsp; and add the following lines into that configuration file.

```properties
app_port=4337
app_port_secure=4338
web_port=4339
insecure_mode_enabled=true
secure_mode_enabled=true
web_mode_enabled=false
server_key=cert_keys/server.key
server_cert=cert_keys/server.crt
ca_cert=cert_keys/ca.crt
allowed_clients=allowed_clients.txt
working_dir=./path/to/work_dir
bind_address=0.0.0.0
```

Note that all the lines in the configuration file are optional. You may omit some lines if they need to get their default values.

<br>

| Property | Description | Accepted values | Default |
|  :----:  | :--------   | :------------   |  :---:  |
| `insecure_mode_enabled` | Whether or not the application listens for unencrypted connections. The values `true` or `1` will enable it, while `false` or `0` will disable it. | `true`, `false`, `1`, `0` (Case insensitive) | `true` |
| `app_port` | The port on which the application listens for unencrypted TCP connections. The application listens on the same port for UDP for network scanning. (Values below 1024 may require superuser/admin privileges) | Any valid, unused port number (1 - 65535) | `4337` |
| `secure_mode_enabled` | Whether or not the application listens for TLS-encrypted connections. The values `true` or `1` will enable it, while `false` or `0` will disable it. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `app_port_secure` | The TCP port on which the application listens for TLS-encrypted connections. (Values below 1024 may require superuser/admin privileges) | Any valid, unused port number (1 - 65535) | `4338` |
| `web_mode_enabled` | Whether or not the application listens for TLS-encrypted connections from web clients if the web mode is available. The values `true` or `1` will enable it, while `false` or `0` will disable it. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `web_port` | The TCP port on which the application listens for TLS-encrypted connections for web clients. This setting is used only if web mode is available. (Values below 1024 may require superuser/admin privileges) | Any valid, unused port number (1 - 65535) | `4339` |
| `server_key` | The TLS private key file of the server. If this is not specified, secure mode (and web mode if available) will be disabled. | Absolute or relative path to the private key file | \<Unspecified\> |
| `server_cert` | The TLS certificate file of the server. If this is not specified, secure mode (and web mode if available) will be disabled. | Absolute or relative path to the server's TLS certificate file | \<Unspecified\> |
| `ca_cert` | The TLS certificate file of the CA that signed the TLS certificate of the server. If this is not specified, secure mode (and web mode if available) will be disabled. | Absolute or relative path to the TLS certificate file of the CA | \<Unspecified\> |
| `allowed_clients` | The text file containing a list of allowed clients (Common Name of client certificate), one name per each line. If this is not specified, secure mode (and web mode if available) will be disabled. | Absolute or relative path to the allowed-clients file | \<Unspecified\> |
| `working_dir` | The working directory where the application should run. All the files, that are sent from a client, will be saved in this directory. It will follow symlinks if this is a path to a symlink. The user running this application should have write access to the directory | Absolute or relative path to an existing directory | . (Current directory) |
| `bind_address` | The address of the interface to which the application should bind when listening for connections. It will listen on all interfaces if this is set to `0.0.0.0` | IPv4 address of an interface in a.b.c.d format or `0.0.0.0` | `0.0.0.0` |

<br>

You may change these values. But it is recommended to keep the port numbers unchanged. If the port numbers are changed, client application configurations may also need to be changed as appropriate to connect to the server.
<br>
If you changed the configuration file, you must restart the server to apply the changes.