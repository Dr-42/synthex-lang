# Synthex Programming Language

Note: Synthex is an ongoing project and the language is not yet finished. The information provided here is based on the current state of the language as of 05-08-2023.(dd/mm/yyyy)
Introduction

Synthex is a programming language designed to be simple, expressive, and efficient. It draws inspiration from various existing programming languages while introducing some unique features that make it stand out.

## Features

- [x] Rust-like Syntax : Synthex adopts a familiar syntax inspired by the Rust programming language, making it easy for developers with Rust, C, C++, or Java experience to quickly get started.
- [x] Strong Typing: Synthex is statically and strongly typed, providing the benefit of early error detection and increased program stability.
- [x] Arrays: Synthex supports arrays, allowing you to work with collections of data efficiently.
- [x] Conditional Statements: You can use if-else statements for decision-making in your programs.
- [x] Loops: Synthex provides while loops, enabling repetitive execution of code blocks based on a condition.
- [ ] For loops
- [ ] Pointers
- [ ] Custom types with structs and enums and such
- [ ] Generic types

Sample Program: Rule 110

```synthex
// rule 110
fnc print(a : str, ...) : void;
fnc get_num() : i32;

fnc main() : i32 {
    print("Enter number of rows: ");
    rows : i32 = get_num();

    arr : [i32; 51];
    arr_n : [i32; 51];

    // Zero out the arrays
    ctr : i32 = 0;
    while (ctr < 51){
        arr[ctr] = 0;
        arr_n[ctr] = 0;
        ctr = ctr + 1;
    }

    // Set the initial conditions
    arr[49] = 1;

    idx : i32 = 0;
    while (idx < rows) {
        sc : i32 = 1;
        // Calculate the next row
        while (sc < 50){
            if (arr[sc - 1] == 1 && arr[sc] == 1 && arr[sc + 1] == 1) {
                arr_n[sc] = 0;
            } elif (arr[sc - 1] == 1 && arr[sc] == 1 && arr[sc + 1] == 0){
                arr_n[sc] = 1;
            } elif (arr[sc - 1] == 1 && arr[sc] == 0 && arr[sc + 1] == 1){
                arr_n[sc] = 1;
            } elif (arr[sc - 1] == 1 && arr[sc] == 0 && arr[sc + 1] == 0){
                arr_n[sc] = 0;
            } elif (arr[sc - 1] == 0 && arr[sc] == 1 && arr[sc + 1] == 1){
                arr_n[sc] = 1;
            } elif (arr[sc - 1] == 0 && arr[sc] == 1 && arr[sc + 1] == 0){
                arr_n[sc] = 1;
            } elif (arr[sc - 1] == 0 && arr[sc] == 0 && arr[sc + 1] == 1){
                arr_n[sc] = 1;
            } elif (arr[sc - 1] == 0 && arr[sc] == 0 && arr[sc + 1] == 0){
                arr_n[sc] = 0;
            }

            sc = sc + 1;
        }
        // Print the row
        i : i32 = 0;
        while (i < 50){
            ch : str;
            if (arr[i] == 1){
                ch = "X";
            } elif (arr[i] == 0){
                ch = " ";
            } else {
                ch = "*";
            }
            print(ch);
            i = i + 1;
        }
        print("%s", "\n");
        ir : i32 = 0;
        while (ir < 50){
            tm : i32 = arr_n[ir];
            arr[ir] = tm;
            ir = ir + 1;
        }
        idx = idx + 1;
    }
    ret 0;
}
```

## Language Status

As of 05-08-2023.(dd/mm/yyyy), Synthex is still a work in progress. The language is evolving, and new features are being added, while existing ones might undergo changes for improvement.

Note: The sample code provided here may not represent the final syntax or semantics of the language.

## Contributing

I welcome contributions from the community to help shape the future of Synthex. Whether it's fixing bugs, proposing new features, or improving documentation, your efforts are valuable.


## Getting Started

As the language is still in development, there is no stable release available yet. However, you can experiment with the current version.

### Compilng the compiler

Clone the repo.

Install the build tool with cargo

```sh
cargo install builder_cpp
```

Build the project

```sh
builder_cpp -b
```

Compile the source code to a .ll file with

```sh
builder_cpp -br --bin-args <filename.syn>
```

Compile the .ll file with clang

```sh
clang -o <bin-name> <filename.syn.ll>
```

If you want to include external functions from a C source code, just define the functions in the C file and compile them to a .o file and pass them to the link phase.

For example, in the above example, the print and get_num functions are imported from C.

To try out the example code :

Create functions.c file
```c
#include <stdarg.h>
#include <stdio.h>

// Wrapper around printf
void print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vprintf(fmt, args);
    va_end(args);
}

int get_num() {
    int num;
    scanf("%d", &num);
    return num;
}
```

Compile and link

```sh
clang -c -o functions.o functions.c
builder_cpp -r --bin-args rule110.syn
clang -o app functions.o rule110.syn.ll
./app
```

## Community

Join our friendly community of developers and language enthusiasts on Discord to discuss ideas, ask questions, and get updates on the progress of Synthex.

## License

Synthex is released under the BSD 2 clause License, granting developers the freedom to use, modify, and distribute the language under certain conditions. Please refer to the license file for more details.

## Acknowledgments

I would like to express our gratitude to the developers of the programming languages that served as inspirations for Synthex, as well as the broader open-source community for their valuable contributions to the field of programming language development.