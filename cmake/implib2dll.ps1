if ($args.Count -ne 1) {
  Write-Host "Only one argument is needed: import library path"
  exit 1
}
$lib = $args[0]
# if ($lib.StartsWith("$")) {
#   exit 0 # generator expression
# }
if (-not (Test-Path "$lib" -PathType Leaf)) {
  $lib = "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64\${lib}"
  # $lib = "${env:WindowsSdkDir}Lib\${env:WindowsSDKVersion}um\x64\${lib}"
}
if (-not (Test-Path "$lib" -PathType Leaf)) {
  $lib = "${lib}.lib"
}
if (Test-Path env:CI) {
  # On CI, dumpbin is not in PATH by default, use either one of the GH Actions that sets up MSVC
  # or one of the following methods to troubleshoot changes:
  # 1) Get-ChildItem -Path "C:\Program Files\Microsoft Visual Studio\" -Recurse -Filter "dumpbin.exe"
  # 2) $env:Path += ";$(Split-Path -Path $(& "${env:programfiles(x86)}\microsoft visual studio\installer\vswhere" -latest -find "**\Hostx64\x64\dumpbin.exe" | Select-Object -Last 1) -Parent)"
  # It takes time, so add within calling environment.
  $env:PATH += ";C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
}
$match = & dumpbin /ALL "$lib" | Select-String -Pattern "DLL name\s+: ([\w.]+)" | Select-Object -First 1
if ($match) {
  $match.Matches[0].Groups[1].Value
} else {
  # Likely a static library.
  Write-Error "Can't find info for $lib"
}
