#!/bin/bash
# Usage: ./export.sh <input.ngs> <output.json>
# Exports a NetGameSim .ngs file to JSON format
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input.ngs> <output.json>"
    exit 1
fi
JAR="$SCRIPT_DIR/../../NetGameSim/target/scala-3.2.2/netmodelsim.jar"
java -cp "$JAR" com.lsc.Export "$1" "$2"