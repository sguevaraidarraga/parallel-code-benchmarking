param(
    [ValidateSet('check', 'quick')][string]$Mode = 'check',
    [string]$Compiler = 'g++'
)
$ErrorActionPreference = 'Stop'
$runner = Join-Path $PSScriptRoot 'run_tests.py'
$runnerArgs = @($runner, $Mode, '--compiler', $Compiler)
if (Get-Command py -ErrorAction SilentlyContinue) {
    & py -3 @runnerArgs
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
    & python @runnerArgs
} else {
    throw 'Python 3 is required. Install it and reopen the terminal.'
}
exit $LASTEXITCODE
