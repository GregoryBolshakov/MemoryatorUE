@echo off
echo *** Memoryator Cleanup Folders 1.0 ***
echo Starting Cleanup...

pushd "%~dp0"

CALL :rmSafe .\.vs
CALL :rmSafe .\Memoryator.sln
CALL :rmSafe .\.vscode
CALL :rmSafe .\Memoryator.code-workspace
CALL :rmSafe .\Plugins\Runtime\HoudiniEngine

IF "%1" == "--keep-config" (
    CALL :rmSafe .\Saved\Autosaves
    CALL :rmSafe .\Saved\Backup
    CALL :rmSafe .\Saved\Cooked
    CALL :rmSafe .\Saved\Shaders
    CALL :rmSafe .\Saved\StagedBuilds
    CALL :rmSafe .\Saved\Temp
) ELSE (
    CALL :rmSafe .\Saved
)

CALL :cleanupIntermediate .
CALL :cleanupIntermediate Plugins\GameAnalytics
CALL :cleanupIntermediate Plugins\MirrorAnimationSystem
CALL :cleanupIntermediate Plugins\UE4PopcornFXPlugin\PopcornFX
CALL :cleanupIntermediate Plugins\VivoxCoreUE4Plugin
CALL :cleanupIntermediate Plugins\Wwise

popd

echo Done.
EXIT /B %ERRORLEVEL%

:cleanupIntermediate
CALL :rmSafe %1\Binaries
CALL :rmSafe %1\Intermediate
EXIT /B 0

:rmSafe
CALL :getPathType %1
IF %ERRORLEVEL% == 0 (
    echo Deleting folder: %1
    rmdir %1 /S /Q
) else if %ERRORLEVEL% == 1 (
    echo Deleting file: %1
    del %1
)
EXIT /B 0

:getPathType
FOR /f "tokens=1,2 delims=d" %%A in ("-%~a1") do if "%%B" neq "" (
    :: Is folder
    EXIT /B 0
) else if "%%A" neq "-" (
    :: Is file
    EXIT /B 1
)
:: Does not exist
EXIT /B 2
