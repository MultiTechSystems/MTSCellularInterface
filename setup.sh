#!/bin/bash

if [ ! -d ATParser ]
then
    git clone http://github.com/ARMmbed/ATParser
fi

if [ ! -d MTS-Utils ]
then
    git clone http://github.com/MultiTechSystems/MTS-Utils
fi

./version.sh

