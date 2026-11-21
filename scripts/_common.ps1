#Requires -Version 5.1
<#
.SYNOPSIS
	Общие функции скриптов запуска тестов RedComplex.

.DESCRIPTION
	Файл не запускается самостоятельно. Остальные скрипты каталога подключают его
	через точку (dot-sourcing) и используют описанные ниже функции.
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-RedComplexRepositoryRoot {
	# Все скрипты лежат в <корень>\scripts, поэтому корень — родитель каталога скрипта.
	return (Split-Path -Parent $PSScriptRoot)
}

function Get-RedComplexTestSuites {
	# Наборы тестов повторяют структуру tests/ и add_test в CMakeLists.
	return @(
		[pscustomobject]@{
			Name        = 'services'
			Description = 'Сервисы предметной области: items и inventories'
			Tests       = @('TestItemsService', 'TestInventoriesService')
		},
		[pscustomobject]@{
			Name        = 'engine'
			Description = 'Движок: парсер DSL, контексты команд, палитра'
			Tests       = @('TestEngine')
		},
		[pscustomobject]@{
			Name        = 'database'
			Description = 'База данных: SQLite, миграции, ленивые узлы'
			Tests       = @('TestDatabasesService', 'TestLazyNodesDataProviderDb')
		},
		[pscustomobject]@{
			Name        = 'system-router'
			Description = 'SystemRouter: узлы, свойства, команды и блоки данных'
			Tests       = @('TestSystemRouter', 'TestSystemRouterCommandsDb')
		},
		[pscustomobject]@{
			Name        = 'ui'
			Description = 'UI-оболочка: зоны, вкладки, темы, редактор кода'
			Tests       = @(
				'TestUiZones',
				'TestDocumentTabs',
				'TestPropertiesPanel',
				'TestClassManager',
				'TestHighlighters',
				'TestCodeEditorSearch'
			)
		},
		[pscustomobject]@{
			Name        = 'basewidgets'
			Description = 'Базовые виджеты: ленивое дерево'
			Tests       = @('TestLazyTreeView', 'TestLazyTreeWidget')
		}
	)
}

function Get-RedComplexTestSuiteNames {
	$names = @('all')
	foreach ($suite in Get-RedComplexTestSuites) {
		$names += $suite.Name
	}
	return $names
}

function Assert-RedComplexTestSuite {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Name
	)

	if ((Get-RedComplexTestSuiteNames) -notcontains $Name) {
		$available = (Get-RedComplexTestSuiteNames) -join ', '
		throw "Неизвестный набор тестов '$Name'. Доступные наборы: $available."
	}
}

function Get-RedComplexTestSuite {
	param(
		[string]$Name = 'all'
	)

	$suites = @(Get-RedComplexTestSuites)

	if ($Name -eq 'all') {
		$tests = @()
		foreach ($suite in $suites) {
			$tests += @($suite.Tests)
		}
		return @($tests | Select-Object -Unique)
	}

	$suite = $suites | Where-Object { $_.Name -eq $Name } | Select-Object -First 1
	if (-not $suite) {
		Assert-RedComplexTestSuite -Name $Name
	}
	return @($suite.Tests)
}

function Resolve-RedComplexBuildDirectory {
	param(
		[string]$BuildDirectory
	)

	$root = Get-RedComplexRepositoryRoot

	if ($BuildDirectory) {
		$candidate = if ([IO.Path]::IsPathRooted($BuildDirectory)) {
			$BuildDirectory
		}
		else {
			Join-Path $root $BuildDirectory
		}

		if (-not (Test-Path -LiteralPath (Join-Path $candidate 'CMakeCache.txt'))) {
			throw "Каталог '$candidate' не содержит CMakeCache.txt. Укажите каталог сконфигурированной сборки в -BuildDirectory."
		}
		return (Resolve-Path -LiteralPath $candidate).Path
	}

	$default = Join-Path $root 'build\x64-Clang-Debug'
	if (Test-Path -LiteralPath (Join-Path $default 'CMakeCache.txt')) {
		return (Resolve-Path -LiteralPath $default).Path
	}

	$found = @()
	$buildRoot = Join-Path $root 'build'
	if (Test-Path -LiteralPath $buildRoot) {
		foreach ($directory in Get-ChildItem -LiteralPath $buildRoot -Directory) {
			if (Test-Path -LiteralPath (Join-Path $directory.FullName 'CMakeCache.txt')) {
				$found += $directory.FullName
			}
		}
		if (Test-Path -LiteralPath (Join-Path $buildRoot 'CMakeCache.txt')) {
			$found += $buildRoot
		}
	}

	if ($found.Count -eq 1) {
		return (Resolve-Path -LiteralPath $found[0]).Path
	}
	if ($found.Count -eq 0) {
		throw 'Не найден сконфигурированный каталог сборки. Укажите его в -BuildDirectory (например, build\x64-Clang-Debug).'
	}

	$variants = $found -join ', '
	throw "Найдено несколько каталогов сборки: $variants. Укажите нужный в -BuildDirectory."
}

function Get-RedComplexCMakeCacheValue {
	param(
		[Parameter(Mandatory = $true)]
		[string]$BuildDirectory,

		[Parameter(Mandatory = $true)]
		[string]$Name
	)

	$cache = Join-Path $BuildDirectory 'CMakeCache.txt'
	if (-not (Test-Path -LiteralPath $cache)) {
		return $null
	}

	$pattern = '^' + [regex]::Escape($Name) + ':[^=]*=(.*)$'
	$match = Select-String -LiteralPath $cache -Pattern $pattern | Select-Object -First 1
	if (-not $match) {
		return $null
	}
	return $match.Matches[0].Groups[1].Value
}

function Find-RedComplexTestExecutable {
	param(
		[Parameter(Mandatory = $true)]
		[string]$BuildDirectory,

		[Parameter(Mandatory = $true)]
		[string]$TestName
	)

	$fileName = $TestName + '.exe'
	$direct = Join-Path $BuildDirectory $fileName
	if (Test-Path -LiteralPath $direct) {
		return $direct
	}

	# Запасной поиск: тест может оказаться в подкаталоге сборки.
	foreach ($subdirectory in @('tests', 'src')) {
		$searchRoot = Join-Path $BuildDirectory $subdirectory
		if (-not (Test-Path -LiteralPath $searchRoot)) {
			continue
		}

		$match = Get-ChildItem -LiteralPath $searchRoot -Filter $fileName -Recurse -File -ErrorAction SilentlyContinue | Select-Object -First 1
		if ($match) {
			return $match.FullName
		}
	}

	return $null
}

function Assert-RedComplexCommand {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Name,

		[string]$Hint
	)

	if (-not (Get-Command -Name $Name -ErrorAction SilentlyContinue)) {
		$message = "Команда '$Name' не найдена в PATH."
		if ($Hint) {
			$message += ' ' + $Hint
		}
		throw $message
	}
}

function Invoke-RedComplexNativeCommand {
	param(
		[Parameter(Mandatory = $true)]
		[string]$FilePath,

		[string[]]$Arguments = @()
	)

	# В Windows PowerShell 5.1 stderr нативной команды при ErrorActionPreference=Stop
	# становится терминирующей ошибкой. На время вызова ослабляем политику,
	# объединяем потоки и приводим записи к тексту, чтобы вывод компилятора и ctest
	# оставался читаемым, а код возврата не терялся.
	$previousPreference = $ErrorActionPreference
	try {
		$ErrorActionPreference = 'Continue'
		& $FilePath @Arguments 2>&1 | ForEach-Object { $_.ToString() } | Out-Host
		return $LASTEXITCODE
	}
	finally {
		$ErrorActionPreference = $previousPreference
	}
}

function Find-RedComplexVsDevCmd {
	$programFilesX86 = ${env:ProgramFiles(x86)}
	if ($programFilesX86) {
		$vswhere = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
		if (Test-Path -LiteralPath $vswhere) {
			$installation = @(& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null)
			if ($LASTEXITCODE -eq 0 -and $installation.Count -gt 0 -and $installation[0]) {
				$candidate = Join-Path ($installation[0].Trim()) 'Common7\Tools\VsDevCmd.bat'
				if (Test-Path -LiteralPath $candidate) {
					return $candidate
				}
			}
		}
	}

	# vswhere может отсутствовать: перебираем известные расположения Visual Studio.
	foreach ($programFiles in @($env:ProgramFiles, ${env:ProgramFiles(x86)})) {
		if (-not $programFiles) {
			continue
		}

		$vsRoot = Join-Path $programFiles 'Microsoft Visual Studio'
		if (-not (Test-Path -LiteralPath $vsRoot)) {
			continue
		}

		$candidates = @()
		foreach ($version in Get-ChildItem -LiteralPath $vsRoot -Directory -ErrorAction SilentlyContinue) {
			foreach ($edition in Get-ChildItem -LiteralPath $version.FullName -Directory -ErrorAction SilentlyContinue) {
				$candidate = Join-Path $edition.FullName 'Common7\Tools\VsDevCmd.bat'
				if (Test-Path -LiteralPath $candidate) {
					$candidates += $candidate
				}
			}
		}

		if ($candidates.Count -gt 0) {
			return $candidates[0]
		}
	}

	return $null
}

function Initialize-RedComplexVsEnvironment {
	if ($env:VSCMD_VER) {
		return
	}

	$vsDevCmd = Find-RedComplexVsDevCmd
	if (-not $vsDevCmd) {
		Write-Warning 'Не удалось найти VsDevCmd.bat Visual Studio; сборка продолжится в текущем окружении.'
		return
	}

	# VsDevCmd задаёт INCLUDE/LIB/PATH, без которых clang-cl не находит системные заголовки.
	$lines = & cmd.exe /c "`"$vsDevCmd`" -arch=x64 -host_arch=x64 >nul 2>&1 && set"
	if ($LASTEXITCODE -ne 0) {
		throw "Не удалось инициализировать окружение Visual Studio через '$vsDevCmd'."
	}

	foreach ($line in $lines) {
		if ($line -match '^([^=]+)=(.*)$') {
			[Environment]::SetEnvironmentVariable($matches[1], $matches[2])
		}
	}
}

function Initialize-RedComplexTestEnvironment {
	param(
		[Parameter(Mandatory = $true)]
		[string]$BuildDirectory,

		[string]$Platform,

		[switch]$Force
	)

	if ($Platform) {
		$env:QT_QPA_PLATFORM = $Platform
	}

	if ($env:QT_PLUGIN_PATH -and -not $Force) {
		return
	}

	$triplet = Get-RedComplexCMakeCacheValue -BuildDirectory $BuildDirectory -Name 'VCPKG_TARGET_TRIPLET'
	if (-not $triplet) {
		$triplet = 'x64-windows'
	}

	# Порядок соответствует штатной Debug-сборке vcpkg из CMakeSettings.json.
	$candidates = @(
		(Join-Path $BuildDirectory "vcpkg_installed\$triplet\debug\Qt6\plugins"),
		(Join-Path $BuildDirectory "vcpkg_installed\$triplet\release\Qt6\plugins"),
		(Join-Path $BuildDirectory "vcpkg_installed\$triplet\Qt6\plugins")
	)

	foreach ($candidate in $candidates) {
		if (Test-Path -LiteralPath $candidate) {
			$env:QT_PLUGIN_PATH = (Resolve-Path -LiteralPath $candidate).Path
			return
		}
	}
}

function Invoke-RedComplexTestBuild {
	param(
		[Parameter(Mandatory = $true)]
		[string]$BuildDirectory,

		[Parameter(Mandatory = $true)]
		[string[]]$Targets,

		[int]$Parallel = 0,

		[switch]$NoVsEnvironment
	)

	Assert-RedComplexCommand -Name 'cmake' -Hint 'Установите CMake 4.1+ и добавьте его в PATH.'

	if (-not $NoVsEnvironment) {
		Initialize-RedComplexVsEnvironment
	}

	$arguments = @('--build', $BuildDirectory, '--target') + @($Targets)
	if ($Parallel -gt 0) {
		$arguments += @('--parallel', $Parallel)
	}
	else {
		$arguments += @('--parallel')
	}

	Write-Host ('Сборка: ' + ($Targets -join ', ')) -ForegroundColor Cyan
	$exitCode = Invoke-RedComplexNativeCommand -FilePath 'cmake' -Arguments $arguments
	return $exitCode
}
