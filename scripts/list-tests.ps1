#Requires -Version 5.1
<#
.SYNOPSIS
	Показывает наборы тестов RedComplex, их состав и состояние сборки.

.DESCRIPTION
	Без -Functions выводит только список целей и признак того, собран ли тест.
	С -Functions дополнительно запускает каждый собранный тест с ключом -functions
	и печатает имена тестовых функций Qt Test.

.EXAMPLE
	.\list-tests.ps1

.EXAMPLE
	.\list-tests.ps1 -Suite ui

.EXAMPLE
	.\list-tests.ps1 -Functions
#>

[CmdletBinding()]
param(
	[string]$Suite,
	[string]$BuildDirectory,
	[switch]$Functions,
	[string]$Platform
)

. (Join-Path $PSScriptRoot '_common.ps1')

if ($Suite) {
	Assert-RedComplexTestSuite -Name $Suite
}

$buildDirectory = Resolve-RedComplexBuildDirectory -BuildDirectory $BuildDirectory

function Get-RedComplexTestFunctionNames {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Executable,

		[Parameter(Mandatory = $true)]
		[string]$Name
	)

	Push-Location -LiteralPath $buildDirectory
	try {
		$output = @(& $Executable -functions 2>&1)
		$exitCode = $LASTEXITCODE
	}
	finally {
		Pop-Location
	}

	if ($exitCode -ne 0) {
		Write-Warning ("Не удалось прочитать функции теста {0} (код {1})." -f $Name, $exitCode)
		return @()
	}

	$names = @()
	foreach ($line in $output) {
		if ($line -match '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(\)\s*$') {
			$names += $matches[1]
		}
	}
	return $names
}

Initialize-RedComplexTestEnvironment -BuildDirectory $buildDirectory -Platform $Platform

$suites = @(Get-RedComplexTestSuites)
if ($Suite -and $Suite -ne 'all') {
	$suites = @($suites | Where-Object { $_.Name -eq $Suite })
}

Write-Host ''
Write-Host ("Каталог сборки: {0}" -f $buildDirectory) -ForegroundColor Cyan
if (-not $Suite) {
	Write-Host ("Наборы: {0}" -f ((Get-RedComplexTestSuiteNames) -join ', '))
}
Write-Host ''

$totalTests = 0
$builtTests = 0

foreach ($testSuite in $suites) {
	Write-Host ("{0} — {1}" -f $testSuite.Name, $testSuite.Description) -ForegroundColor Cyan

	foreach ($testName in $testSuite.Tests) {
		$totalTests++
		$executable = Find-RedComplexTestExecutable -BuildDirectory $buildDirectory -TestName $testName

		if ($executable) {
			$builtTests++
			$state = 'собран'
			if ($Functions) {
				$functionNames = @(Get-RedComplexTestFunctionNames -Executable $executable -Name $testName)
				$state += (', функций: {0}' -f $functionNames.Count)
			}
		}
		else {
			$state = 'не собран'
			$functionNames = @()
		}

		Write-Host ("  {0,-32} {1}" -f $testName, $state)

		if ($Functions -and $executable) {
			foreach ($functionName in $functionNames) {
				Write-Host ("    {0}" -f $functionName)
			}
			if ($functionNames.Count -eq 0) {
				Write-Host '    (функции не найдены)'
			}
		}
	}

	Write-Host ''
}

Write-Host ("Итого: наборов — {0}, тестов — {1}, собрано — {2}." -f $suites.Count, $totalTests, $builtTests)

if ($builtTests -lt $totalTests) {
	Write-Host 'Несобранные тесты можно собрать командой scripts\build-tests.ps1.' -ForegroundColor Yellow
}
