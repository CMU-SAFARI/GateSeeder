# GateSeeder


## Table of Contents
- [Installation & General usage](#install)
- [Use Cases](#usecases)
- [Key Idea](#idea)
- [Benefits of Genome-on-Diet](#results)
- [Using Genome-on-Diet](#usage)
- [Directory Structure](#directory)
- [Getting help](#contact)
- [Citing Genome-on-Diet](#cite)

## <a name="install"></a>Installation & General usage

### Cloning the repository
```sh
git clone https://github.com/CMU-SAFARI/GateSeeder
```
### Building the xclbin files
To build the `xclbin` files for the three type of datasets run `make` in the `device` directory. You will need Vitis and `rustc` to be installed on your computer.

### Building the host/indexing/profiling
To build the host/indexing/profiling program just run `make` in the root directory. You will need `xrt` to be installed on your computer.

## <a name="usecases"></a>Use Cases

```sh
# Illumina sequences

## Indexing

./GateSeeder_index -t 32 -w 11 -k 21 -f 50 GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta index.sfi

## Mapping

./GateSeeder -t 32 -d 10 -e -c 0 -s device/GateSeeder_illumina.xclbin index.sfi D1_S1_L001_R1_001-017.fastq -o output.paf

# HiFi sequences

## Indexing

./GateSeeder_index -t 32 -w 19 -k 19 -f 1 GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta index.sfi

## Mapping

./GateSeeder -t 32 -d 5000 device/GateSeeder_hifi.xclbin index.sfi m64011_190830_220126.fastq -o output.paf

# ONT sequences

## Indexing

./GateSeeder_index -t 32 -w 10 -k 15 -f 10 GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta index.sfi

## Mapping

./GateSeeder -t 32 -d 950 device/GateSeeder_ont.xclbin index.sfi HG002_ONT-UL_GIAB_20200204_1000filtered_2Mreads.fastq -o output.paf

```

##  <a name="contact"></a>Getting Help
If you have any suggestion for improvement, new applications, or collaboration, please contact julien at eudine dot ch
If you encounter bugs or have further questions or requests, you can raise an issue at the [issue page][issue].

## <a name="cite"></a>Citing GateSeeder

If you use GateSeeder in your work, please cite:

[issue]: https://github.com/CMU-SAFARI/GateSeeder/issues
