# dbg.ps1 — send one command to a running Mother3Recomp TCP debug server.
#
# Launch the game with the server on (starts PAUSED; send continue):
#   .\build\Mother3RecompEN.exe --window --tcp 19894      (JP build: 19893)
#
# Examples:
#   .\tools\dbg.ps1 continue
#   .\tools\dbg.ps1 '{"cmd":"get_registers"}'
#   .\tools\dbg.ps1 '{"cmd":"symbol","addr":"0x08070BB4"}'
#   .\tools\dbg.ps1 '{"cmd":"savestate_load","path":"variants/mother3en/roms/mother3_en.state2"}'
#   .\tools\dbg.ps1 '{"cmd":"read_iwram","addr":"0x03000000","len":32768}' -Out iwram.bin
#
# -Out writes the response's hex "data" field as raw bytes; otherwise the JSON
# line is printed. Protocol reference: gbarecomp/TCP.md.
param(
    [Parameter(Mandatory = $true, Position = 0)][string]$Command,
    [int]$Port = 19894,
    [string]$Out,
    [int]$TimeoutMs = 20000
)
$ErrorActionPreference = "Stop"
if ($Command -notmatch '^\s*\{') { $Command = "{`"cmd`":`"$Command`"}" }

$c = New-Object Net.Sockets.TcpClient
$c.Connect("127.0.0.1", $Port)
$c.ReceiveTimeout = $TimeoutMs
$s = $c.GetStream()
$w = New-Object IO.StreamWriter($s); $w.NewLine = "`n"; $w.AutoFlush = $true
$w.WriteLine($Command)
$line = (New-Object IO.StreamReader($s)).ReadLine()
$c.Close()

if ($Out) {
    $j = $line | ConvertFrom-Json
    if (-not $j.ok) { throw "command failed: $($j.error)" }
    $hex = [string]$j.data
    $bytes = New-Object byte[] ($hex.Length / 2)
    for ($i = 0; $i -lt $bytes.Length; $i++) {
        $bytes[$i] = [Convert]::ToByte($hex.Substring($i * 2, 2), 16)
    }
    [IO.File]::WriteAllBytes((Join-Path (Get-Location) $Out), $bytes)
    Write-Output "wrote $($bytes.Length) bytes to $Out"
} else {
    Write-Output $line
}
