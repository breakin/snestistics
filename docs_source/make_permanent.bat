REM Consider moving into cmake
call make.bat html
rmdir /s /q build\html\_sources
del build\.buildinfo
rmdir /s /q ..\docs\user-guide
mkdir ..\docs\user-guide
xcopy /E /I build\html ..\docs\user.guide
echo Make sure _static gets moved! > ..\docs\user-guide\.nojekyll