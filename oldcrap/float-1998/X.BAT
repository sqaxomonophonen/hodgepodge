@echo off
wpp386 float.cpp
if ERRORLEVEL=1 goto end
wlink file float
if ERRORLEVEL=1 goto end
float
:end
80x50
