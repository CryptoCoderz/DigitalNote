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
       brew install boost
       brew install miniupnpc
       brew install openssl
       brew install automake
       brew install autoconf
       brew install libtool
       brew install qrencode
       ```
   2. Install Berkeley-DB@6
       Download:
       
       ```
       curl -OL http://download.oracle.com/berkeley-db/db-6.2.32.tar.gz
       ```
       
       Unzip:
       ```
       tar -xf db-6.2.32.tar.gz
       ```
       Build:
       ```
       cd db-6.2.32/build_unix                                              &&
       ../dist/configure --prefix=/usr/local/Cellar/berkeley-db@6.2.32      \
                         --enable-cxx                                       &&
       make
       ```
      
       If you get compile errors in `atomic.c` you need to apply a small patch and run the 'build' command above again: 
       ```
        cd ../..
        curl -OL https://raw.githubusercontent.com/macports/macports-ports/cb92cb90bdc7fb90212e928db32172546eca0f5b/databases/db60/files/patch-src_dbinc_atomic.h
        mv patch-src_dbinc_atomic.h db-6.2.32
        cd db-6.2.32
        patch -s -p0 < patch-src_dbinc_atomic.h
        cd ..
       ```
      
       Install:
       ```
        inside db-6.2.32/build_unix folder run:
      
        sudo make install
       ```
   3. You might need to create a symlink if `openssl/sha.h` or any other header from openssl/ folder cannot be found when building DigitalNoted:
       ```
       cd /usr/local/include
       ln -s ../opt/openssl/include/openssl .
       ```
      Your compiler will search in this directory (one of many standard directories) and find the header file sha.h via the shortcut link.
   4. Now create a symlink to miniupnpc files since it is installed in a folder without a version appended and source files are expecting a version:
       ```
       cd /usr/local/include
       ln -s ../opt/miniupnpc/include/miniupnpc ./miniupnpc-2.1
       ``` 
   5. Check the versions of dependencies in src/makefile.osx and amend to match yours if required 
        
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


DigitalNote-qt: Qt5 GUI Release for DigitalNote
-----------------------------------------

1. Install dependencies:
   ```
   brew install qrencode
   brew install qt5
   brew install protobuf
   brew install python2.7
   sudo easy_install appscript
   ```
2. Link qt5:
   ```
   brew link qt5 --force
   ```
3. Run in the ./DigitalNote-2
   ```
   qmake RELEASE=1 USE_UPNP=1 USE_QRCODE=1 DigitalNote.pro
   make
   python2.7 contrib/macdeploy/macdeployqtplus DigitalNote-Qt.app -add-qt-tr da,de,es,hu,ru,uk,zh_CN,zh_TW -dmg -fancy contrib/macdeploy/fancy.plist
   ```
 