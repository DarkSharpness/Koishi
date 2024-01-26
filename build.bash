#!/bin/bash

cd include/antlr
antlr4 -visitor -no-listener -o ../generated -Dlanguage=Cpp MxLexer.g4 MxParser.g4
