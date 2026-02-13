#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
LIB_DIR="$SCRIPT_DIR/../lib"

echo "export LD_LIBRARY_PATH=$LIB_DIR:\$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH="$LIB_DIR:${LD_LIBRARY_PATH}"
