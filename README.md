# VCard Parser

This repository contains a C library for parsing, validating, and writing vCard 4.0 files as specified in [RFC 6350](https://tools.ietf.org/html/rfc6350). The library implements the functions declared in `VCParser.h` and uses a custom linked list implementation provided in `LinkedListAPI.c`. Recent enhancements include improved error handling, stricter validation of properties, and additional functionality for writing and validating Card objects.

## Table of Contents
- [Features](#features)
- [Enhanced Functionality](#enhanced-functionality)
  - [Enhanced Validation and Error Handling](#enhanced-validation-and-error-handling)
  - [WriteCard and ValidateCard Functions](#writecard-and-validatecard-functions)
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

## Enhanced Functionality

### Enhanced Validation and Error Handling

Recent updates ensure that:
- Reserved properties (such as VERSION, BDAY, and ANNIVERSARY) are not incorrectly placed in the optional properties list.
- The **N** property is validated to have exactly five components.
- The **KIND** property (if present) appears at most once.
- The DateTime structures for BDAY and ANNIVERSARY are checked for internal consistency.
- All functions now checked for memory allocation failures and free all allocated memory upon error, allowing improved stability and memory safety.

### WriteCard and ValidateCard Functions

- **writeCard(const char *fileName, const Card *obj):**  
  Serializes a Card object to a file in valid vCard format with CRLF line endings. It avoids line folding to simplify automated testing. It returns `OK` on success or `WRITE_ERROR` if any file writing issues occur.

- **validateCard(const Card *obj):**  
  Validates a Card object against both the internal structure requirements and a subset of the vCard format rules. It ensures that all required properties (like FN and a proper VERSION) are present, verifies the structure and cardinality of properties, and checks that DateTime fields adhere to expected formats. It returns `OK` if valid or an appropriate error code (`INV_CARD`, `INV_PROP`, or `INV_DT`) otherwise.

## Directory Structure

Relevant file structure for the project (ignoring instructions and test files):

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
- **CFLAGS:** `-Wall -Wextra -std=c11 -g`
- **LDFLAGS:** `-shared`

The resulting shared library (`libvcparser.so`) is moved to the `bin` directory.

To clean up build, run:

```bash
make clean
```

## Running the Test Harness

A test harness (e.g., `./test1pre`) is provided to verify the functionality of the parser (for Intel Systems). Before running the test harness, ensure that the shared library is found by the dynamic linker. Since the Makefile moves the shared library to the `bin` directory, run:

```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./bin
./test1pre
```
