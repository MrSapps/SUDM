#### Simple Universal Decompilation Machine [![Build Status](https://travis-ci.org/paulsapps/SUDM.svg?branch=master)](https://travis-ci.org/paulsapps/SUDM)  [![Coverity Scan Build Status](https://scan.coverity.com/projects/3895/badge.svg)](https://scan.coverity.com/projects/3895) [![Coverage Status](https://img.shields.io/coveralls/paulsapps/SUDM.svg)](https://coveralls.io/r/paulsapps/SUDM)

This project aims to provide a decompiler framework for simple byte code languages that do not have specific structuring instructions (while/for/if).

The idea is that you will implement a disassmebler that groups instructions into bespoke types (conditional jumps, unconditional jumps, load store, etc). Then SUDM will apply the generic structing. You'll also implement a code generator that will map a scripting languages kernel functions onto the target output, for example if the game calls function 0x28 you can output this as the string "do_save();". Additionally you can also apply extra CFG post processing.

The initial goal is to support disassmeblers for Final Fantasy 7,8 and 9 "field" script bytecode, and a LUA back end (currently only C output is supported).

This work is heavily based on Michael Madsen's decompiler work for SCUMMVM's decompiler.
