# JPEG 2000 to IMF DCDM (jid)

_THIS IS EXPERIMENTAL SOFTWARE_

Wraps JPEG 2000 codestreams into an Image Track File as specified in SMPTE ST 2067-21 (App 2E) and (proposed) SMPTE ST 2067-40 (App 4)

## Prerequisites

* xerces (required by asdcplib)
* openssl (required by asdcplib)
* boost::program_options
* cmake

## Known limitations

* Does not support interlaced images
* Supported input formats: MJC codestream sequence and J2C single codestream

## Examples uses

### Performance comparison

The following use the demonstration encoder/decoder available at <https://kakadusoftware.com/downloads/>.

#### JPEG 2000 Part 1

```
time ( kdu_v_compress -i ~/Downloads/tiff-files/mer_shrt_23976_vdm_sdr_rec709_g24_3840x2160_20170913_12bit_DCDM.00090000.tif+100 -o - \
  Simf="{6,0,rev}" -in_prec 12M -num_threads 1 \
  | dcdm2imf --format MJC --out ~/Downloads/part1-r.mxf )
```

#### High-throughput JPEG 2000

```
time ( kdu_v_compress -i ~/Downloads/tiff-files/mer_shrt_23976_vdm_sdr_rec709_g24_3840x2160_20170913_12bit_DCDM.00090000.tif+100 -o - Corder="CPRL" Clevels="{6}" \
Cprecincts="{256,256},{256,256},{256,256},{256,256},{256,256},{256,256},{128,128}" ORGgen_tlm="{3}" ORGtparts=C Cblk="{32,32}" \
-num_threads 1 Creversible=yes Cmodes=HT -in_prec 12M | dcdm2imf --format MJC --out ~/Downloads/part15-r.mxf )
```

### Typical use

#### JPEG 2000 Part 1

```
kdu_v_compress -i ~/Downloads/tiff-files/mer_shrt_23976_vdm_sdr_rec709_g24_3840x2160_20170913_12bit_DCDM.00090000.tif+100 -o - Simf="{6,0,rev}" -in_prec 12M \
  | dcdm2imf --format MJC --out ~/Downloads/part1-r.mxf
```

#### High-throughput JPEG 2000

```
kdu_v_compress -i ~/Downloads/tiff-files/mer_shrt_23976_vdm_sdr_rec709_g24_3840x2160_20170913_12bit_DCDM.00090000.tif+100 -o - Corder="CPRL" Clevels="{6}" \
Cprecincts="{256,256},{256,256},{256,256},{256,256},{256,256},{256,256},{128,128}" ORGgen_tlm="{3}" ORGtparts=C Cblk="{32,32}" Creversible=yes Cmodes=HT -in_prec 12M \
  | dcdm2imf --format MJC --out ~/Downloads/part15-r.mxf
```

## Ubuntu build instructions

```
sudo apt-get install libboost-all-dev
sudo apt-get install libxerces-c-dev
sudo apt-get install libssl-dev
sudo apt-get install cmake

git clone --recurse-submodules https://github.com/sandflow/jid.git
cd jid
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

#tests

ctest
```

## MacOS build instructions

```
brew install boost
brew install xerces-c
brew install openssl
brew install cmake

git clone --recurse-submodules https://github.com/sandflow/jid.git
cd jid
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DOpenSSLLib_include_DIR=/usr/local/opt/openssl@1.1/include \ 
  -DOpenSSLLib_PATH=/usr/local/opt/openssl@1.1/lib/libcrypto.dylib ..
make

#tests

ctest
```

## Microsoft Visual Studio build instructions (MSVC)

* install OpenSSL binaries from https://slproweb.com/products/Win32OpenSSL.html
* install boost binaries from
https://sourceforge.net/projects/boost/files/boost-binaries/1.72.0/
* download xerces c++ from http://xerces.apache.org/mirrors.cgi and
build using MSVC
* configure and generate an MSVC project using CMake GUI, setting:
  * Boost_INCLUDE_DIR
  * XercescppLib_PATH
  * XercescppLib_Debug_PATH
  * XercescppLib_include_DIR
  * OpenSSLLib_PATH
  * OpenSSLLib_include_DIR
