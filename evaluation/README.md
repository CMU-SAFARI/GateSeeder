# Sequencing Reads:
The short reads (Illumina), the accurate long reads (HiFi), and the ultra-long reads (ONT) are obtained from the NIST's Genome-in-a-Bottle (GIAB)  project: 
```
https://ftp-trace.ncbi.nlm.nih.gov/ReferenceSamples/giab/data/AshkenazimTrio/HG002_NA24385_son/
```

## PacBio HiFi reads:
```
https://ftp-trace.ncbi.nlm.nih.gov/ReferenceSamples/giab/data/AshkenazimTrio/HG002_NA24385_son/PacBio_CCS_15kb_20kb_chemistry2/reads/m64011_190830_220126.fastq.gz
```


## ONT ultra-long reads:
We consider only the first 2 million reads whose length is greater than or equal 1000 bp (using NanoFilt --length 1000)
```
https://ftp-trace.ncbi.nlm.nih.gov/ReferenceSamples/giab/data/AshkenazimTrio/HG002_NA24385_son/Ultralong_OxfordNanopore/guppy-V3.4.5/HG002_ONT-UL_GIAB_20200204.fastq.gz
```

## Illumina 250bp reads:
```
https://ftp-trace.ncbi.nlm.nih.gov/ReferenceSamples/giab/data/AshkenazimTrio/HG002_NA24385_son/NIST_Illumina_2x250bps/reads/D1_S1_L001_R1_001.fastq.gz
```
---

To run the evaluation scripts please set the environment variable `DATA` to the path where you store all your data.
