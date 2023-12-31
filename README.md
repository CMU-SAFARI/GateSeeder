# GateSeeder: Near-memory CPU-FPGA Acceleration of Short and Long Read Mapping
Read mapping for short and long reads is still computationally expensive and usually a major bottleneck in most genomic analyses. Hence, various prior works aim to alleviate this bottleneck by accelerating pairwise sequence alignment using software optimizations, heuristic algorithms, and hardware accelerators (GPUs, FPGAs, ASICs). However, the performance of read mapping is mainly limited by the performance of three key computational steps: Index Querying, Seed Chaining, and Sequence Alignment. The first step is dominated by how fast and frequent it accesses the main memory (i.e., memory-bound), while the latter two steps are dominated by how fast the CPU can compute their computationally-costly dynamic programming algorithms (i.e., compute-bound). Accelerating these three steps by exploiting new algorithms and new hardware devices is essential to accelerate most genome analysis pipelines that widely use read mapping. 

We introduce **GateSeeder**, the first near-memory CPU-FPGA co-design for alleviating both the compute-bound and memory-bound bottlenecks in short and long-read mapping. GateSeeder exploits near-memory computation capability provided by modern FPGAs that couple a reconfigurable compute fabric with high-bandwidth memory (HBM) to overcome the memory-bound and compute-bound bottlenecks. GateSeeder also introduces a new lightweight algorithm for finding the potential matching segment pairs. Given the large body of work on proposing new algorithms and hardware accelerators for sequence alignment over the last five decades, we focus on accelerating all other computational steps of read mapping. These steps (indexing, seeding, index querying) are not used only in read mapping, but also fundamentally used in many applications that use an index data structure (e.g., hash table), such as genomic similarity, taxonomy profiling, genome assembly, and mapping to pangenome reference. Using real ONT, HiFi, and Illumina sequences, we experimentally demonstrate that GateSeeder outperforms Minimap2, without performing sequence alignment, by up to 40.3×, 4.8×, and 2.3×, respectively. When performing read mapping with sequence alignment, GateSeeder outperforms Minimap2 by 1.15-4.33× (using KSW2) and by 1.97-13.63× (using WFA-GPU).

GateSeeder is fully synthesizable, open-source, and ready-to-be-used on real hardware (e.g., a computer connected with an Xilinx Alveo U55C card featuring the Virtex Ultrascale+ XCU55C). We provide the source code, scripts, and evaluation data on our GitHub page to reproduce all the graphs and results we show in our submission.


## Table of Contents
- [Installation & General usage](#install)
- [Use Cases](#usecases)
- [Directory Structure](#directory)
- [Getting help](#contact)
- [Citing GateSeeder](#cite)

## <a name="install"></a>Installation & General usage

### Cloning the repository
```sh
git clone https://github.com/CMU-SAFARI/GateSeeder
```
### Building the xclbin files
To build the `xclbin` files for the three types of sequencing reads run `make` in the `device` directory. You will need to install the Vitis™ HLS tool and the `rustc` compiler for the Rust programming language on your computer.

### Building the host/indexing/profiling
To build the host/indexing/profiling program just run `make` in the root directory. You will need to install Xilinx Runtime library (XRT) on your computer.

### Usage
```sh
# Indexing
./GateSeeder_index [OPTION...] <target.fasta> <index.sfi>

# Mapping
./GateSeeder [OPTION...] <bitstream.xclbin> <index.sfi> <query.fa>
```


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

##  <a name="directory"></a>Directory Structure:
```
GateSeeder
.
├── device
│   ├── config
│   ├── scripts
│   └── src
├── evaluation
│   └── scripts
├── host
│   └── src
├── indexing
│   └── src
├── plots
│   ├── accuracy
│   ├── profile
│   ├── scripts
│   ├── speedup
│   └── transfer_processing_time
├── profiling
│   └── src
├── test
└── util
    ├── include
    └── src
```            
1. In the "device" directory, you will find the source code of the FPGA implementation of GateSeeder, the current build targets the Xilinx Alveo U55C FPGA but it can be adapted for other Xilinx FPGAs.
2. In the "evaluation" directory, you will find all the shell scripts and commands we used to run the experimental evaluations we presented in the paper.
3. In the "host" directory, you will find the source code of the host implementation of GateSeeder.
4. In the "plots" directory, you will find all the source code to generate the plots we presented in the paper based on the results of the "evaluation" and "profiling".
5. In the "profiling" directory, you will find all the profiling host source code we used to measure the kernel and transfer metrics.
6. In the "util" directory, you will find some utility source code.


##  <a name="contact"></a>Getting Help
If you have any suggestions for improvement, new applications, or collaboration, please get in touch with: julien at eudine dot fr and mealser at gmail dot com
If you encounter bugs or have further questions or requests, you can raise an issue at the [issue page][issue].

## <a name="cite"></a>Citing GateSeeder

If you use GateSeeder in your work, please cite:
> Julien Eudine and Mohammed Alser, Gagandeep Singh, Can Alkan, Onur Mutlu
> "GateSeeder: Near-memory CPU-FPGA Acceleration of Short and Long Read Mapping"
> (2023) [link](https://arxiv.org/abs/2309.17063)

Below is bibtex format for citation.

```bibtex
@article{GateSeeder2023,
    author = {Julien Eudine and Mohammed Alser, Gagandeep Singh, Can Alkan, Onur Mutlu},
    title = "{GateSeeder: Near-memory CPU-FPGA Acceleration of Short and Long Read Mapping}",
    year = {2023},
}
```

[issue]: https://github.com/CMU-SAFARI/GateSeeder/issues
