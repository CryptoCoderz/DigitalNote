DigitalNote [XDN] integration/staging tree
=====================================

http://www.digitalnote.biz

What is the DigitalNote [XDN] Blockchain?
---------------------------

The DigitalNote [XDN] Blockchain is an experimental smart contract platform protocol that enables 
instant payments to anyone, anywhere in the world in a private, secure manner. 
DigitalNote [XDN] uses peer-to-peer blockchain technology developed by DigitalNote to operate
with no central authority: managing transactions, execution of contracts, and 
issuing money are carried out collectively by the network. DigitalNote [XDN] is the name of 
open source software which enables the use of this protocol.

Specifications and General info
------------------
DigitalNote uses libsecp256k1,
			  libgmp,
			  Boost1.63,
			  OR Boost1.57,  
			  Openssl1.02k,
			  Berkeley DB 6.2.23,
			  QT5.8 to compile


Block Spacing: 5 Minutes
Stake Minimum Age: 15 Confirmations (PoS-v3) | 30 Minutes (PoS-v2)

Port: 18092
RPC Port: 18094


BUILD LINUX
-----------
1) git clone https://github.com/DigitalNoteXDN/DigitalNote-1

2) cd XDN/src

3) sudo make -f makefile.unix            # Headless

(optional)

4) strip DigitalNoted

5) sudo cp DigitalNoted /usr/local/bin

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
