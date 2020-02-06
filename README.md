# JPEG 2000 to IMF DCDM (jid)

_THIS IS EXPERIMENTAL SOFTWARE AND CONTAINS BUGS. DO NOT USE FOR PRODUCTION._

Wraps JPEG 2000 codestreams into an Image Track File as specified in ST 2067-40

## Prerequisites

* xerces (required by asdcplib)
* opensll (required by asdcplib)
* boost::program_options
* cmake

## Typical use

```
kdu_v_compress.exe -i ~/Downloads/tiff-files/mer_shrt_23976_vdm_sdr_rec709_g24_3840x2160_20170913_12bit_DCDM.00090000.tif+100 -o - Creversible=yes Cmodes=HT -in_prec 12M \
  | dcdm2imf.exe --format MJC --out d:/part15-r.mxf
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

