param([string]$projectdir,
      [string]$outputpath,
      [string]$configuration,
      [string]$name);

$includedir = "Include\"
$copyPath = "$projectdir\..\Source\ThirdParty\OpenCVWrapper\"

New-Item "$copyPath\Binaries\$configuration\Dynamic\" -ItemType Directory -ea 0
New-Item "$copyPath\Binaries\$configuration\Static\" -ItemType Directory -ea 0
New-Item "$copyPath\Include" -ItemType Directory -ea 0
New-Item "$copyPath\OpenCV\Include" -ItemType Directory -ea 0

robocopy "$outputpath" "$copyPath\Binaries\$configuration\Dynamic\" "$name.dll" /is
robocopy "$outputpath" "$copyPath\Binaries\$configuration\Dynamic\" "$name.pdb" /is
robocopy "$outputpath" "$copyPath\Binaries\$configuration\Static\" "$name.lib" /is
robocopy "$outputpath" "$copyPath\Binaries\$configuration\Static\" "$name.lib" /is
robocopy "$outputpath" "$copyPath\Binaries\$configuration\Static\" "$name.lib" /is
robocopy "$projectdir\Include" "$copyPath\Include\"
robocopy "$projectdir\ThirdParty\OpenCV\Include\" "$copyPath\OpenCV\Include\" /E

robocopy "$outputpath" "$projectdir\..\..\..\Binaries\ThirdParty\Win64\" "$name.dll" /is
if ($configuration -eq "Debug") {
    robocopy "$outputpath" "$projectdir\..\..\..\Binaries\ThirdParty\Win64\" "$name.pdb" /is
}