## Simple Universal Decompilation Machine [![Build Status](https://travis-ci.org/paulsapps/SUDM.svg?branch=master)](https://travis-ci.org/paulsapps/SUDM)
====

This project aims to provide a decompiler framework for simple byte code languages that do not have specific structuring instructions (while/for/if).

The idea is what you will implement a disassmebler that groups instructions into bespoke types (conditional jumps, unconditional jumps, load store, etc). Then SUDM will apply the generic structing. 

The initial goal is to support disassmeblers for Final Fantasy 7,8 and 9 "field" script bytecode, and a LUA back end (currently only C output is supported).

This work is heavily based on Michael Madsen's decompiler work for SCUMMVM's decompiler.
