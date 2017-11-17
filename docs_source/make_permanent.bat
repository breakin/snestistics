REM Consider moving into cmake
call make.bat html
rmdir /s /q build\html\_sources
rmdir /s /q ..\docs
mkdir ..\docs
xcopy /E /I build\html ..\docs