ECHO OFF

IF NOT EXIST ATParser (
    git clone http://github.com/ARMmbed/ATParser
)

IF NOT EXIST MTS-Utils (
    git clone http://github.com/MultiTechSystems/MTS-Utils
)

version.bat

