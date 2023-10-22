#!/bin/bash

# 1. Get current format script's path
current_path=$(cd `dirname $0`; pwd)

# 2. Runs astyle with the full set of formatting options
$current_path/astyle \
        --style=otbs \
        --indent=spaces=4 \
        --convert-tabs \
        --align-pointer=name            \
        --align-reference=name \
        --keep-one-line-statements \
        --pad-header\
        --pad-oper \
        "$@"
