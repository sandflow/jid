The files in this directory were generated as described below from the [Netflix Meridian test material](https://media.xiph.org/video/derf/meridian/MERIDIAN_SHR_C_EN-XX_US-NR_51_LTRT_UHD_20160909_OV), which is licensed under the [BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) license.

```
kdu_compress -i mer_shrt_23976_vdm_sdr_rec709_g24_3840x2160_20170913_12bit_DCDM.00090000.tif Simf=\{6,0,rev\} -fprec 12M -o part1.j2c
```

```
kdu_compress -i mer_shrt_23976_vdm_sdr_rec709_g24_3840x2160_20170913_12bit_DCDM.00090000.tif Corder="CPRL" Clevels="{6}" \
  Cprecincts="{256,256},{256,256},{256,256},{256,256},{256,256},{256,256},{128,128}" ORGgen_tlm="{3}" ORGtparts=C \
  Cblk="{32,32}" Creversible=yes Cmodes=HT -fprec 12M -o part15.j2c
```

```
kdu_v_compress -i mer_shrt_23976_vdm_sdr_rec709_g24_3840x2160_20170913_12bit_DCDM.00090000.tif+1 -o - \
  Simf=\{6,0,rev\} -in_prec 12M > part1.mjc
```

```
kdu_v_compress.exe -i mer_shrt_23976_vdm_sdr_rec709_g24_3840x2160_20170913_12bit_DCDM.00090000.tif+1 -o - Corder="CPRL" \
  Clevels="{6}" Cprecincts="{256,256},{256,256},{256,256},{256,256},{256,256},{256,256},{128,128}" ORGgen_tlm="{3}" \
  ORGtparts=C Cblk="{32,32}" Creversible=yes Cmodes=HT -in_prec 12M > part15.mjc
```

The `kdu*` demonstration executables are available at <http://kakadusoftware.com/>.