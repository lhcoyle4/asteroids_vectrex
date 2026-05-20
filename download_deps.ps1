# PowerShell script to download SDL2 and SDL2_mixer MinGW developer libraries

$sdlVersion = "2.30.3"
$mixerVersion = "2.8.2"

$sdlUrl = "https://github.com/libsdl-org/SDL/releases/download/release-$sdlVersion/SDL2-devel-$sdlVersion-mingw.tar.gz"
$mixerUrl = "https://github.com/libsdl-org/SDL_mixer/releases/download/release-$mixerVersion/SDL2_mixer-devel-$mixerVersion-mingw.tar.gz"

Write-Host "Creating thirdparty folder..."
if (!(Test-Path "thirdparty")) {
    New-Item -ItemType Directory -Force -Path "thirdparty"
}

Write-Host "Downloading SDL2..."
Invoke-WebRequest -Uri $sdlUrl -OutFile "thirdparty/sdl2.tar.gz"

Write-Host "Downloading SDL2_mixer..."
Invoke-WebRequest -Uri $mixerUrl -OutFile "thirdparty/sdl2_mixer.tar.gz"

Write-Host "Extracting SDL2..."
tar -xf thirdparty/sdl2.tar.gz -C thirdparty

Write-Host "Extracting SDL2_mixer..."
tar -xf thirdparty/sdl2_mixer.tar.gz -C thirdparty

Write-Host "Cleaning up tar.gz files..."
Remove-Item "thirdparty/sdl2.tar.gz"
Remove-Item "thirdparty/sdl2_mixer.tar.gz"

Write-Host "Dependencies downloaded and extracted successfully."
