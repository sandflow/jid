# JPEG 2000 to IMF DCDM (jid)

_THIS IS EXPERIMENTAL SOFTWARE. DO NOT USE FOR PRODUCTION._

Wraps JPEG 2000 codestreams into an Image Track File as specified in ST 2067-40

## Prerequisites

* xerces (required by asdcplib)
* opensll (required by asdcplib)
* boost::program_options
* cmake

## Known limitations

* Supported image characteristics: DCDM XYZ @ 4:4:4
* Supported input formats: MJC codestream sequence and J2C single codestream

## Examples uses

### Performance comparison

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
```

## MacOS build instructions

```
brew install boost
brew install xerces-c
brew install openssl
brew link openssl --force
brew install cmake

git clone --recurse-submodules https://github.com/sandflow/jid.git
cd jid
mkdir build
cd build
cmake -DOpenSSLLib_include_DIR=/usr/local/opt/openssl@1.1/include -DOpenSSLLib_PATH=/usr/local/opt/openssl@1.1/lib/libcrypto.dylib ..
make
```
