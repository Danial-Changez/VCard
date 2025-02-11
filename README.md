# VCard Parser

This repository contains a C library for parsing vCard 4.0 files as specified in [RFC 6350](https://tools.ietf.org/html/rfc6350). The library implements the functions declared in `VCParser.h` and uses a custom linked list implementation provided in `LinkedListAPI.c`.

## Table of Contents
- [Features](#features)
- [Directory Structure](#directory-structure)
- [Build Instructions](#build-instructions)
- [Running the Test Harness](#running-the-test-harness)

## Features

- **vCard 4.0 Parsing:** Supports parsing vCard files according to RFC 6350.
- **Line Folding & CRLF Handling:** Validates that physical lines end with CRLF and properly unfolds folded lines.
- **Composite Property Support:** Splits composite values (e.g., the N property) by the ';' delimiter while preserving empty tokens.
- **Date-Time Parsing:** Constructs `DateTime` structures for BDAY and ANNIVERSARY properties.
- **Error Handling:** Returns precise error codes when the file, card, or properties are invalid.
- **Custom Linked List:** Uses a custom doubly linked list to store properties and their parameters.

## Directory Structure
Relevant file structure for the program (ignoring module and test files).
```
├── bin/
│   └── libvcparser.so         # The built shared library
├── include/
│   ├── VCParser.h             # Public header for the vCard parser
│   └── LinkedListAPI.h        # Public header for the linked list API
├── src/
│   ├── VCParser.c             # Implementation of the vCard parser
│   └── LinkedListAPI.c        # Implementation of the linked list API
├── Makefile                   # Build instructions for the shared library
└── README.md                  # This file
```

## Build Instructions

A Makefile is provided to compile the shared library. To build the library, run:

```bash
make parser
```

This command compiles `VCParser.c` and `LinkedListAPI.c` using the following flags:
- **CFLAGS:** `-Wall -Wextra -std=c11 -fPIC -g`
- **LDFLAGS:** `-shared`

The resulting shared library (`libvcparser.so`) is moved to the `bin` directory.

To clean up build, run:

```bash
make clean
```

## Running the Test Harness

A test harness (e.g., `./test1pre`) is provided to verify the functionality of the parser (for Intel Systems). Before running the test harness, ensure that the shared library is found by the dynamic linker. For example, if your shared library is in the `bin` directory, run:

```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./bin
./test1pre
```
