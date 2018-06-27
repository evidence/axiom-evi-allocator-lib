# AXIOM 3rd level Memory Allocator based on LMM

----

**NOTE: This repository is a submodule of axiom-evi project. Please clone the
master project from https://github.com/evidence/axiom-evi**

This repository contains the implementation of the AXIOM 3rd level Memory
Allocator based on List Memory Manager (LMM).

The code is organized with the following directories:

 * ./src
    + contains the implementation of the Axiom Allocator
 * ./test
    + contains some tests of the API


## How to compile

```
# Use the axiom-evi/scripts/Makefile to compile the AXIOM allocator lib
cd axiom-evi/scripts

# Configure all modules (required only the first time)
make MODE=aarch64 configure

# compile the AXIOM allocator lib and tests
make allocator-lib
```

## How to run tests
```
test/test*

```

