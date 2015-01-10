Tetris
======

[![Build Status](https://travis-ci.org/koturn/Tetris.png)](https://travis-ci.org/koturn/Tetris)

Terminal tetris.

This tetris works on terminal.

You can build with both MSVC and gcc!


## Usage

```sh
$ ./tetris
```


## Build

#### With GCC

Use [Makefile](Makefile).

```sh
$ make
```

#### With MSVC

Use [msvc.mk](msvc.mk).
[msvc.mk](msvc.mk) is written for nmake.

###### Release version

```sh
> nmake /f msvc.mk
```

###### Debug version

```sh
> nmake /f msvc.mk DEBUG=true
```


## Dependent libraries

- [TermUtil](https://github.com/koturn/TermUtil)


## LICENSE

This software is released under the MIT License, see [LICENSE](LICENSE).
