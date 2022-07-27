# Clip Share
### Copy on one device. Paste on another device

<br>
This is the server that runs in the background.
<br>
<br>

## Building
This needs,
* gcc
* make

### Dependencies
#### Linux
The following development libraries are required.
* libx11
* libxmu
* libpng
* libssl (Optional. This is only needed for building with the web server enabled.)

They can be installed with the following command:

* on Debian or Ubuntu based distros,

      sudo apt-get install libx11-dev libxmu-dev libpng-dev libssl-dev

* on Redhat or Fedora based distros,

      sudo yum install libX11-devel libXmu-devel libpng-devel openssl-devel

* on Arch based distros,

      sudo pacman -S libx11 libxmu libpng openssl

#### Windows
The following development libraries are required.
* libz
* libpng16
* libssl (provided by openssl)

### Compiling
Run the following command to make the executable file.

    make

This will generate the executable which is named clip_share (or clip_share.exe on Windows).

To compile with the web server enabled, (Currently, this is supported only on Linux)

    make web

This will generate the web server enabled executable which is named clip_share_web.

<br>

## Allow through firewall
This server listens on ports,

* ``4337`` &nbsp;&nbsp; ``TCP`` &nbsp;&nbsp; : &nbsp;&nbsp; For application traffic (not encrypted)
* ``4337`` &nbsp;&nbsp; ``UDP`` &nbsp;&nbsp; : &nbsp;&nbsp; For network scanning
* ``4338`` &nbsp;&nbsp; ``TCP`` &nbsp;&nbsp; : &nbsp;&nbsp; For application traffic over TLS (encrypted)
* ``4339`` &nbsp;&nbsp; ``TCP`` &nbsp;&nbsp; : &nbsp;&nbsp; For the web server (if the web server is available)

You may need to allow incoming connections to the above ports for the client to connect to the server.

<br>

## How to use
* Run the server

    When the server is started, it will not open any window. Instead, it will run in the background.
    If the program is started from the terminal, it should return immediately (server will continue to run in the background).
    If something goes wrong, a server_err.log file will be created and it will contain what went wrong.

* Connect the client application

    Open the client application on any other device on the same network.
    If the client supports network scanning, the server can be easily found. Otherwise, enter the server's IPv4 address to the client.
    Now the client should be able to share clipboard data, files, and get images from the server.
    Note that the client should be allowed through the firewall as mentioned in the above section.

<br>

## Configuration
* Create a file named &nbsp; ``clipshare.conf`` &nbsp; and add the following lines into that configuration file

      app_port=4337
      app_port_secure=4338
      web_port=4339
      server_key=cert_keys/server.key
      server_cert=cert_keys/server.crt
      ca_cert=cert_keys/ca.crt
      allowed_clients=allowed_clients.txt

You may ommit some lines if they need to get their default values.

* ``server.key`` is the SSL private key file of server.
* ``server.crt`` is the SSL certificate file of server.
* ``ca.crt`` is the SSL certificate file of the CA which signed the certificate of the server.
* ``allowed_clients`` is a text file containing a list of allowd clients (Common Name of client certificate) one name per each line

You may change those values. But it is recommended to keep the port numbers unchanged.