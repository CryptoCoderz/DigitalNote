DigitalNote [XDN] 2014-2018 (CryptoNote Base), 2018-2020 (Current) integration/staging tree
===========================================================================================

http://www.digitalnote.biz

What is the DigitalNote [XDN] Blockchain?
-----------------------------------------
*TODO: Update documentation regarding implemented tech as this section is out of date and much progress and upgrades have been made to mentioned sections...*

### Overview
DigitalNote is a blockchain project with the goal of offering secured messaging, Darksend, masternodes and an overall pleasing experience to the user.

### Blockchain Technology
The DigitalNote [XDN] Blockchain is an experimental smart contract platform protocol that enables 
instant payments to anyone, anywhere in the world in a private, secure manner. 
DigitalNote [XDN] uses peer-to-peer blockchain technology developed by DigitalNote to operate
with no central authority: managing transactions, execution of contracts, and 
issuing money are carried out collectively by the network. DigitalNote [XDN] is the name of 
open source software which enables the use of this protocol.

### Custom Difficulty Retarget Algorithm “VRX”
VRX is designed from the ground up to integrate properly with the Velocity parameter enforcement system to ensure users no longer receive orphan blocks.

### Velocity Block Constraint System
Ensuring Insane stays as secure and robust as possible the CryptoCoderz team have implemented what's known as the Velocity block constraint system. This system acts as third and final check for both mined and peer-accepted blocks ensuring that all parameters are strictly enforced.

### Wish (bmw512) Proof-of-Work Algorithm
Wish or bmw512 hashing algorithm is utilized for the Proof-of-Work function and also replaces much of the underlying codebase hashing functions as well that normally are SHA256. By doing so this codebase is able to be both exponentially lighter and more secure in comparison to reference implementations.

### Echo512 Proof-of-Stake Algorithm
DigitalNote's proof of stake system utilizes Echo512 which is a super lightweight and secure hashing algorithm.

Specifications and General info
------------------
DigitalNote uses 

	libsecp256k1,
	libgmp,
	Boost1.68, OR Boost1.58,  
	Openssl1.02r,
	Berkeley DB 6.2.32,
	QT5.12.1,
	to compile


General Specs

	Block Spacing: 2 Minutes
	Stake Minimum Age: 15 Confirmations (PoS-v3) | 30 Minutes (PoS-v2)
	Port: 18092
	RPC Port: 18094

BUILD MacOS
-----------
See /docs/build-osx.md

BUILD LINUX
-----------
### Compiling DigitalNote "SatoshiCore" daemon on Ubunutu 18.04 LTS Bionic
### Note: guide should be compatible with other Ubuntu versions from 14.04+

### Become poweruser
```
sudo -i
```
### CREATE SWAP FILE FOR DAEMON BUILD (if system has less than 2GB of RAM)
```
cd ~; sudo fallocate -l 3G /swapfile; ls -lh /swapfile; sudo chmod 600 /swapfile; ls -lh /swapfile; sudo mkswap /swapfile; sudo swapon /swapfile; sudo swapon --show; sudo cp /etc/fstab /etc/fstab.bak; echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

### Dependencies install
```
cd ~; sudo apt-get install -y ntp git build-essential libssl-dev libdb-dev libdb++-dev libboost-all-dev libqrencode-dev libcurl4-openssl-dev curl libzip-dev; apt-get update -y; apt-get install -y git make automake build-essential libboost-all-dev; apt-get install -y yasm binutils libcurl4-openssl-dev openssl libssl-dev; sudo apt-get install -y libgmp-dev; sudo apt-get install -y libtool;
```

### Dependencies build and link
```
cd ~; wget http://download.oracle.com/berkeley-db/db-6.2.32.NC.tar.gz; tar zxf db-6.2.32.NC.tar.gz; cd db-6.2.32.NC/build_unix; ../dist/configure --enable-cxx --disable-shared; make; sudo make install; sudo ln -s /usr/local/BerkeleyDB.6.2/lib/libdb-6.2.so /usr/lib/libdb-6.2.so; sudo ln -s /usr/local/BerkeleyDB.6.2/lib/libdb_cxx-6.2.so /usr/lib/libdb_cxx-6.2.so; export BDB_INCLUDE_PATH="/usr/local/BerkeleyDB.6.2/include"; export BDB_LIB_PATH="/usr/local/BerkeleyDB.6.2/lib"
```

### GitHub pull (Source Download)
```
cd ~; git clone https://github.com/DigitalNoteXDN/DigitalNote-2 DigitalNote
```

### Build DigitalNote daemon
```
cd ~; cd ~/DigitalNote/src; chmod a+x obj; chmod a+x leveldb/build_detect_platform; chmod a+x secp256k1; chmod a+x leveldb; chmod a+x ~/DigitalNote/src; chmod a+x ~/DigitalNote; make -f makefile.unix USE_UPNP=-; cd ~; cp -r ~/DigitalNote/src/DigitalNoted /usr/local/bin/DigitalNoted;
```

### (Optional) Build DigitalNote-QT (GUI wallet) on Linux 

**All previous steps must be completed first.**

If you recompiling some other time you don't have to repeat previous steps, but need to define those variables. Skip this command if this is your first build and previous steps were performed in current terminal session.
```
export BDB_INCLUDE_PATH="/usr/local/BerkeleyDB.6.2/include"; export BDB_LIB_PATH="/usr/local/BerkeleyDB.6.2/lib"
```

With UPNP:

```
cd ~; cd ~/DigitalNote; qmake -qt=qt5; make
```

**Recommended Without** UPNP:

```
cd ~; cd ~/DigitalNote; qmake -qt=qt5 USE_UPNP=-; make
```



### Create config file for daemon
```
cd ~; sudo ufw allow 18092/tcp; sudo ufw allow 18094/tcp; sudo ufw allow 22/tcp; sudo mkdir ~/.XDN; cat << "CONFIG" >> ~/.XDN/DigitalNote.conf
listen=1
server=1
daemon=1
testnet=0
rpcuser=XDNrpcuser
rpcpassword=SomeCrazyVeryVerySecurePasswordHere
rpcport=18094
port=18092
rpcconnect=127.0.0.1
rpcallowip=127.0.0.1
CONFIG
chmod 700 ~/.XDN/DigitalNote.conf; chmod 700 ~/.XDN; ls -la ~/.XDN
```

### Run DigitalNote daemon
```
cd ~; DigitalNoted; DigitalNoted getinfo
```

### Troubleshooting
### for basic troubleshooting run the following commands when compiling:
### this is for minupnpc errors compiling

```
make clean -f makefile.unix USE_UPNP=-
make -f makefile.unix USE_UPNP=-
```
### Updating daemon in bin directory
```
cd ~; cp -r ~/DigitalNote/src/DigitalNoted /usr/local/bin
```

License
-------

DigitalNote [XDN] is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/DigitalNoteXDN/DigitalNote-1) are created
regularly to indicate new official, stable release versions of DigitalNote [XDN].

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

The developer [mailing list](https://lists.linuxfoundation.org/mailman/listinfo/bitcoin-dev)
should be used to discuss complicated or controversial changes before working
on a patch set.

Developer Discord can be found at https://discord.gg/eSb7nEx.

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](/doc/unit-tests.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`

There are also [regression and integration tests](/qa) of the RPC interface, written
in Python, that are run automatically on the build server.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.
