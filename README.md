# Severus

A tool for revealing scheduling patterns in NUMA architectures.
See our paper at PACT 2019: *Unfair Scheduling Patterns in NUMA Architectures*.


## Dependencies

You will need a Linux machine with:
* [GCC](https://packages.ubuntu.com/search?keywords=gcc&searchon=names)
* [Boost.Program_options](https://packages.ubuntu.com/search?keywords=libboost-program-options-dev&searchon=names)
* [gnuplot](https://packages.ubuntu.com/search?keywords=gnuplot&searchon=names)
* [libnuma](https://packages.ubuntu.com/search?keywords=libnuma-dev&searchon=names)
* a NUMA architecture (for interesting results)


## Getting Started

Clone or download this repository, then move to its directory.

    git clone https://github.com/cmuparlay/severus
    cd severus

You can now run experiments!
All compilation is done automatically as needed by the experiment scripts.
The main entry points are `paper.sh` (high-level interface) and `go.sh` (low-level interface).
You might start with:

    ./paper.sh easy

This will output results in a new `output` directory.
For more detailed usage information, run `./paper.sh --help` and `go.sh --help`.

## Publication

This repository is also accessible through Zenodo, DOI 10.5281/zenodo.3360044.

It can be found at https://doi.org/10.5281/zenodo.3360044.

Please use this DOI when citing this repositoy.
