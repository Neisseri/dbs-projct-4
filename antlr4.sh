#!/bin/sh

cd src &&
  java -jar ../3rd_party/antlr-4.8-complete.jar \
    -Dlanguage=Cpp -visitor -no-listener \
    grammar/SQL.g4 -o . -package grammar
