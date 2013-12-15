@echo off
cls
:: cl /nologo /D_CRT_SECURE_NO_WARNINGS /MT /Ox /GL /Zi /W4 /WX /wd4530 /wd4100 input.cc /link /LTCG /OUT:input.exe
cl /nologo /D_CRT_SECURE_NO_WARNINGS /Zi /W4 /WX /wd4530 /wd4100 input.cc /link /OUT:input.exe
