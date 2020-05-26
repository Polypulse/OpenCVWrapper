param([string]$projectdir,
     [string]$outputpath,
     [string]$configuration,
     [string]$name);

$targetpath = [IO.File]::ReadAllText(".\PostBuildTargetPath.txt")
echo "Copying from: $outputpath to: $targetpath"

$includedir = "Include\"
robocopy "$projectdir$includedir\" "$targetpath\Include\" /is

robocopy "$outputpath" "$targetpath\Binaries\$configuration\Dynamic\" "$name.dll" /is
robocopy "$outputpath" "$targetpath\Binaries\$configuration\Dynamic\" "$name.pdb" /is
robocopy "$outputpath" "$targetpath\Binaries\$configuration\Static\" "$name.lib" /is

robocopy "$outputpath" "$targetpath\..\..\..\Binaries\ThirdParty\Win64\" "$name.dll" /is
if ($configuration -eq "Debug") {
    robocopy "$outputpath" "$targetpath\..\..\..\Binaries\ThirdParty\Win64\" "$name.pdb" /is
}