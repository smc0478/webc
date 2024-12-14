# Webc web server library
WebC is web server library developed in C.

## Introdution
WebC is a web server that I am developing for fun.
The goal is to implement most of the features found in other web frameworks.
It provides a shared library(libwebc).


## Installing
```bash
make
```

## Example
Please refer to the 'example' folder.

### build
```bash
make
```

### test
```bash
source env_set.sh
./[path/to/executable file].out
```

```bash
curl 127.0.0.1:8080
# output: hello world!
```

## Dependency
Required:
- C compiler
- make
- GCC

## TODO LIST
- features(ex. cookie, session, file, etc)
- logging for debugging
- example
- test code
- refactoring
- optimize
- documentation
