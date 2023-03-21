REM Delete Engine.ini as it currently contains absolute paths for game plugins. (TODO: Fix this...)
copy %~dp0..\Saved\Config\Windows\Engine.ini %~dp0..\Saved\Config\Windows\Engine.temp
del %~dp0..\Saved\Config\Windows\Engine.ini

REM Now fill the DDC using the stadard Unreal commandlet
%~dp0"C:\UnrealEngine\Engine\Binaries\Win64\UnrealEditor.exe" ..\Memoryator.uproject -run=DerivedDataCache -fill -LogCmds="LogClass off"

copy %~dp0..\Saved\Config\Windows\Engine.temp %~dp0..\Saved\Config\Windows\Engine.ini
del %~dp0..\Saved\Config\Windows\Engine.temp