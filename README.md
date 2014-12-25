## Simple Universal Decompilation Machine [![Build Status](https://travis-ci.org/paulsapps/SUDM.svg?branch=master)](https://travis-ci.org/paulsapps/SUDM)
====

This project aims to provide a decompiler framework for simple byte code languages that do not have specific structuring instructions (while/for/if).

The idea is what you will implement a front end which translates the instructions to SUDM's internal instruction format. Then SUDM will apply the structing and any required optimizations. Finally you can implement a new backend which writes out the source code, or simply choose an existing backend.

The initial goal is to support front ends for Final Fantasy 7,8 and 9 "field" script bytecode, and a LUA back end. This means that SUDM's internal instruction set will have no notion of exceptions, although support for this could be added later.

