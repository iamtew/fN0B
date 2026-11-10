# fN0B – Justfile (Projucer + MSBuild)

set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

JUCE_PATH := env_var_or_default("JUCE_PATH", env_var("USERPROFILE") + "\\JUCE")
PROJUCER  := JUCE_PATH + "\\Projucer.exe"
PROJECT   := "fN0B.jucer"
SOLUTION  := "Builds\\VisualStudio2022\\fN0B.sln"
# Bare `msbuild` is not on PATH with Build Tools alone.
# CI sets MSBUILD to the runner's Visual Studio; this default stays the local Build Tools.
MSBUILD   := env_var_or_default("MSBUILD", "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\MSBuild\\Current\\Bin\\MSBuild.exe")

default:
    @just --list

projucer:
    {{PROJUCER}} {{PROJECT}}

resave:
    {{PROJUCER}} --resave {{PROJECT}}
    # OneDrive can lag the sln behind "Finished saving".
    $n=0; while (-not (Test-Path '{{SOLUTION}}') -and $n -lt 20) { Start-Sleep -Milliseconds 250; $n++ }

# Debug standalone must already exist. Interactive desktop only.
docs-screenshots:
    & './scripts/docs-screenshots.ps1'

# assets/logo.png is the Affinity export. Source of truth. Never write that file.
# Derived: docs/logo.png (320, Lanczos EWA + unsharp), Builds/.../icon.ico per size the same way.
# `just resave` does not refresh icon.ico when the PNG changes.
docs-logo:
    magick assets/logo.png -background none -colorspace RGB -filter Lanczos +distort Resize 320x320 -unsharp 0x0.5+0.5+0.008 -colorspace sRGB PNG32:docs/logo.png

icon: docs-logo
    $dir = 'Builds\VisualStudio2022'; $pngs = @(); foreach ($s in 256,128,64,48,32,16) { $out = Join-Path $dir ("_ico{0}.png" -f $s); magick assets/logo.png -background none -colorspace RGB -filter Lanczos +distort Resize ${s}x${s} -unsharp 0x0.5+0.5+0.008 -colorspace sRGB PNG32:$out; $pngs += $out }; magick @pngs (Join-Path $dir 'icon.ico'); $pngs | Remove-Item -Force
    Get-ChildItem Builds\VisualStudio2022 -Recurse -Filter resources.res | Remove-Item -Force

# Debug builds VST3 and the standalone (inspection / screenshots). Release is VST3 only.
build-debug: resave
    & '{{MSBUILD}}' {{SOLUTION}} /p:Configuration=Debug /p:Platform=x64 /m

build-release: resave
    & '{{MSBUILD}}' {{SOLUTION}} /t:fN0B_VST3 /p:Configuration=Release /p:Platform=x64 /m

build-dist: build-release
    if (Test-Path dist) { Remove-Item dist -Recurse -Force }
    New-Item dist -ItemType Directory | Out-Null
    $ver = ([xml](Get-Content '{{PROJECT}}')).JUCERPROJECT.version; Compress-Archive -Path 'Builds\VisualStudio2022\x64\Release\VST3\fN0B.vst3' -DestinationPath "dist\fN0B-$ver-win-x64-vst3.zip" -Force
    just build-msi

# Release binaries must already exist. First build downloads WiX.
# `dotnet` on PATH when it has an SDK, otherwise %USERPROFILE%\.dotnet.
build-msi:
    $ver = ([xml](Get-Content '{{PROJECT}}')).JUCERPROJECT.version; $root = (Get-Location).Path; $dotnet = 'dotnet'; if (-not ((& dotnet --list-sdks) 2>$null)) { $dotnet = Join-Path $env:USERPROFILE '.dotnet\dotnet.exe'; if (-not (Test-Path $dotnet)) { throw 'build-msi needs the .NET 8 SDK on PATH, or dotnet.exe in %USERPROFILE%\.dotnet' } }; New-Item -ItemType Directory -Force -Path dist | Out-Null; & $dotnet build installer\fN0B.wixproj -c Release -p:ProductVersion=$ver -p:Vst3Dir="$root\Builds\VisualStudio2022\x64\Release\VST3\fN0B.vst3" -p:IconDir="$root\Builds\VisualStudio2022"; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; $msi = Get-ChildItem installer\bin -Recurse -Filter fN0B.msi | Select-Object -First 1; Copy-Item $msi.FullName "dist\fN0B-$ver-win-x64-installer.msi" -Force

clean:
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Debug /p:Platform=x64
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Release /p:Platform=x64

rebuild: clean build-debug

vs:
    start {{SOLUTION}}

check:
    cmd /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat`" >nul && cl /nologo /EHsc /std:c++17 /I src tests\fn0b_check.cpp /Fo:tests\fn0b_check.obj /Fe:tests\fn0b_check.exe && tests\fn0b_check.exe"
