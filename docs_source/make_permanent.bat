REM Consider moving into cmake
call make.bat html
rmdir /s /q build\html\_sources
del build\.buildinfo
rmdir /s /q ..\docs
mkdir ..\docs
xcopy /E /I build\html ..\docs
echo Make sure _static gets moved! > ..\docs\.nojekyll