# CShell

## Introduction

This is a refactored version of a shell I wrote for my Systems Programming class.

## Getting Started

### Prerequisites

You must have `gcc` or some form of C compiler installed in order to run this code.

### Functionality
It supports:

* Certain Bash Built-in Commands (`cd` and `exit`)
* Redirection (`<`, `>`, `>>`)
* Piping (`|`)
* Backgrounding (`&`)
* Chaining
    * Consecutive (`;`)
    * Simultaneous (`&`)

Additional features, such as Regex and conditionals, are planned.

An example command:

```bash
wc < shell.c > test.txt & ls | wc | egrep 0; sleep 5&
```

### Compilation

After downloading the file, run:
```bash
gcc -g -o shell shell.c # -g only necessary if debugging with gdb
```

### Running The Shell

After compiling the code, run:
```bash
./shell
```

## Author

Kevin Karnani
