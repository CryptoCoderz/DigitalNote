Copyright (c) 2009-2012 Bitcoin Developers
Distributed under the MIT/X11 software license, see the accompanying file
license.txt or http://www.opensource.org/licenses/mit-license.php.  This
product includes software developed by the OpenSSL Project for use in the
OpenSSL Toolkit (http://www.openssl.org/).  This product includes cryptographic
software written by Eric Young (eay@cryptsoft.com) and UPnP software written by
Thomas Bernard.


Mac OS X DigitalNoted build instructions
Laszlo Hanyecz <solar@heliacal.net>
Douglas Huff <dhuff@jrbobdobbs.org>


See readme-qt.rst for instructions on building DigitalNote QT, the
graphical user interface.

- Tested on 10.5 and 10.6 intel and 10.15.2.  
- PPC is not supported because it's big-endian.

All of the commands should be executed in Terminal.app.. it's in
/Applications/Utilities

You need to install XCode with all the options checked so that the compiler and
everything is available in /usr not just /Developer 
You can get the current version from http://developer.apple.com


1. Clone the github tree to get the source code

    ```git clone http://github.com/DigitalNotedev/DigitalNote DigitalNote``` 

2. Install dependencies using Homebrew
    1. Install dependencies:
    ```
   brew install boost@1.59
   brew install berkeley-db4           
   brew install miniupnpc@2.1
   brew install https://github.com/tebelorg/Tump/releases/download/v1.0.0/openssl.rb
   brew install automake
   brew install autoconf
   brew install libtool
   brew install qrencode
   ```
   2. You might need to create a symlink if `openssl/sha.h` or any other header from openssl/ folder cannot be found when building DigitalNoted:
   ```
   cd /usr/local/include
   ln -s ../opt/openssl/include/openssl .
   ```
   Your compiler will search in this directory (one of many standard directories) and find the header file sha.h via the shortcut link.
   
   3. Now create a symlink to miniupnpc files since it is installed in a folder without a version appended and source files are expecting a version:
   ```
   cd /usr/local/include
   ln -s ../opt/miniupnpc/include/miniupnpc ./miniupnpc-2.1
   ``` 
   4. Check the versions of dependencies in src/makefile.osx and amend to match yours if required 
        
3.  Now you should be able to build DigitalNoted:

    ```
    cd DigitalNote-2/src
    make -f makefile.osx
    ```

Run:
  `./DigitalNoted --help` 
for a list of command-line options.
  
Run
  `./DigitalNoted -daemon`
to start the DigitalNote daemon.
  
Run
  `./DigitalNoted help`
When the daemon is running, to get a list of RPC commands
