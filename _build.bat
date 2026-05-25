@echo off
:: _build.bat — FULIGIN full build pipeline.
:: Kills any running instance, cleans objects, compiles the native Windows
:: executable, then stages and pushes source changes to GitHub.
:: GitHub Actions will handle the Emscripten/WASM build automatically.
cd /d "%~dp0"
echo.
echo ============================================================
echo  FULIGIN BUILD PIPELINE
echo ============================================================
echo  Working directory: %CD%
echo.

:: ── 1. Kill running instance + clean ─────────────────────
taskkill /f /im fuligin.exe >nul 2>nul
timeout /t 1 /nobreak >nul
if exist fuligin.exe ( del fuligin.exe )
del /q /f src\*.o src\*.wasm.o 2>nul

:: ── 2. Build Windows exe ──────────────────────────────────
echo [1/2] Building Windows executable...
make 2>&1
set MAKE_ERR=%errorlevel%
echo.

if %MAKE_ERR% NEQ 0 (
    echo BUILD FAILED - make returned error %MAKE_ERR%. Aborting.
    exit /b 1
)
echo BUILD SUCCESS - fuligin.exe ready
echo.

:: ── 3. Git commit + push (triggers WASM CI on GitHub) ─────
echo [2/2] Staging source changes for GitHub...
echo       ^(GitHub Actions will build WASM automatically^)

:: Stage only source and config — never binaries or thirdparty
git add src\            2>nul
git add include\        2>nul
git add Makefile        2>nul
git add _build.bat      2>nul
git add .gitignore      2>nul
git add .gitattributes  2>nul
git add .github\        2>nul

:: Check if anything is actually staged
git diff --cached --quiet
if %errorlevel% == 0 (
    echo No source changes staged - skipping commit.
    echo Run the game: fuligin.exe
    goto :done
)

:: Commit with a short timestamp (PowerShell, since wmic is unavailable on Win11)
for /f %%I in ('powershell -Command "Get-Date -Format \"yyyy-MM-dd HH:mm\""') do set STAMP=%%I

git commit -m "FULIGIN update %STAMP%"

git push origin master
if %errorlevel% == 0 (
    echo.
    echo Pushed to GitHub. CI will:
    echo   1. Build WASM with Emscripten
    echo   2. Deploy to gh-pages branch
    echo   3. Serve at: https://lhcoyle4.github.io/fuligin/
) else (
    echo.
    echo Push failed - check git credentials or network.
    echo Local build still succeeded: fuligin.exe
)

:done
echo.
echo ============================================================
echo  Done.
echo ============================================================
