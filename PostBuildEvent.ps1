param([string]$projectdir,
     [string]$outputpath,
     [string]$configuration,
     [string]$name);

$targetpath = [IO.File]::ReadAllText(".\PostBuildTargetPath.txt")
echo "Copying from: $outputpath to: $targetpath"

$includedir = "Include\"
robocopy "$projectdir$includedir\" "$targetpath\Include\"

robocopy "$outputpath" "$targetpath\Binaries\$configuration\Dynamic\" "$name.pdb"
robocopy "$outputpath" "$targetpath\Binaries\$configuration\Static\" "$name.lib"
robocopy "$outputpath" "$targetpath\Binaries\$configuration\Static\" "$name.lib"