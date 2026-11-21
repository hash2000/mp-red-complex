#Requires -Version 5.1
<#
.SYNOPSIS
	Собирает тесты RedComplex без запуска.

.DESCRIPTION
	Полезно для CI и для проверки компиляции: собираются только цели выбранного
	набора, ctest не запускается. Для запуска используйте run-tests.ps1.

.EXAMPLE
	.\build-tests.ps1

.EXAMPLE
	.\build-tests.ps1 -Suite database -Parallel 4
#>

[CmdletBinding()]
param(
	[string]$Suite = 'all',
	[string]$BuildDirectory,
	[int]$Parallel = 0,
	[switch]$NoVsEnvironment
)

. (Join-Path $PSScriptRoot '_common.ps1')

Assert-RedComplexTestSuite -Name $Suite

$buildDirectory = Resolve-RedComplexBuildDirectory -BuildDirectory $BuildDirectory
$targets = @(Get-RedComplexTestSuite -Name $Suite)
if ($targets.Count -eq 0) {
	throw "Набор тестов '$Suite' пуст."
}

$buildExitCode = Invoke-RedComplexTestBuild -BuildDirectory $buildDirectory -Targets $targets -Parallel $Parallel -NoVsEnvironment:$NoVsEnvironment

if ($buildExitCode -eq 0) {
	Write-Host ''
	Write-Host ("Тесты собраны: {0} ({1} шт.)." -f $Suite, $targets.Count) -ForegroundColor Green
}
else {
	Write-Host ''
	Write-Host ("Сборка завершилась с кодом {0}." -f $buildExitCode) -ForegroundColor Red
}

exit $buildExitCode
