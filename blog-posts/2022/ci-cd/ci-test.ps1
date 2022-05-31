<#(Rn.BuildScriptHelper){
	"version": "1.0.106"
}#>
param (
  [Parameter(Mandatory=$false)]
  [string] $rootDir = $PSScriptRoot,

  [Parameter(Mandatory=$false)]
  [string] $testCsprojPattern = "*T1.Tests.csproj",

  [Parameter(Mandatory=$false)]
  [string] $configuration = "Release",

  [Parameter(Mandatory=$false)]
  [string] $frameworkVersion = "net6.0"
)

$rootDir               = [IO.Path]::GetFullPath((Join-Path $rootDir "\..\"));
$sourceDir             = Join-Path $rootDir "test\";
$publishDir            = Join-Path $rootDir "artifacts\";
$toolsDir              = Join-Path $rootDir "tools\";
$testPublishDir        = Join-Path $publishDir "t1-publish\";
$testResultsDir        = Join-Path $publishDir "t1-results\";
$testCoverageDir       = Join-Path $publishDir "t1-coverage\";
$coverletExe           = Join-Path $toolsDir "coverlet.exe";
$reportGeneratorExe    = Join-Path $toolsDir "reportgenerator.exe"

# =============================================================================
# Info Dumping
# =============================================================================
#
Write-Output ("=============================================================");
Write-Output ("= ci-test.ps1 :: information");
Write-Output ("=============================================================");
Write-Output ("= rootDir            : $rootDir");
Write-Output ("= sourceDir          : $sourceDir");
Write-Output ("= publishDir         : $publishDir");
Write-Output ("= toolsDir           : $toolsDir");
Write-Output ("= testPublishDir     : $testPublishDir");
Write-Output ("= testResultsDir     : $testResultsDir");
Write-Output ("= testCoverageDir    : $testCoverageDir");
Write-Output ("= coverletExe        : $coverletExe");
Write-Output ("= reportGeneratorExe : $reportGeneratorExe");
Write-Output ("=============================================================");

# ==============================================================
# Cleanup
# ==============================================================
#
$cleanupDirectories = @();
$cleanupDirectories += $testPublishDir;
$cleanupDirectories += $testCoverageDir;
$cleanupDirectories += $testResultsDir;
 
foreach ($cleanupDirectory in $cleanupDirectories) {
  if (Test-Path $cleanupDirectory) {
    Remove-item $cleanupDirectory -Recurse -Force | Out-Null;
  }
}

# ==============================================================
# Install tooling
# ==============================================================
#
$installCmd = "";

if(!(Test-Path $coverletExe)) {
  Write-Host "Installing: coverlet.console"
  $installCmd = "dotnet tool install coverlet.console --tool-path $toolsDir";
  Invoke-Expression -Command $installCmd -ErrorAction 'Continue';
}

if(!(Test-Path $reportGeneratorExe)) {
  Write-Host "Installing: reportgenerator.exe"
  $installCmd = "dotnet tool install dotnet-reportgenerator-globaltool --tool-path `"$toolsDir`"";
  Invoke-Expression -Command $installCmd -ErrorAction 'Continue';
}

# ==============================================================
# Discover and Build test projects
# ==============================================================
#
$testProjects = Get-ChildItem -Path $sourceDir -Include $testCsprojPattern -Recurse -File

if ($testProjects.count -eq 0) {
  throw "No files matched the $testCsprojPattern pattern. The script cannot continue."
}

Write-Output ("Found " + $testProjects.count + " test project(s) to build and run");
foreach ($testProject in $testProjects) {
  # ---------------------------------------------------- >>
  # Generate required paths
  $dllFileName         = $testProject.BaseName + ".dll";
  $testSrcDir          = Join-Path $testProject.Directory.FullName "\";
  $testBinDir          = Join-Path $testSrcDir "bin\";
  $testBinConfigDir    = Join-Path $testBinDir ($configuration + "\");
  $testFrameworkDir    = Join-Path $testBinConfigDir ($frameworkVersion + "\");
  $testDllFile         = Join-Path $testFrameworkDir $dllFileName;
  $csprojFullPath      = $testProject.FullName;

  # ---------------------------------------------------- >>
  # Restore and build the project
  $buildCmd = "dotnet build `"$csprojFullPath`" --configuration $configuration";
  Write-Output ("(INFO) Building test project :: $buildCmd");
  Invoke-Expression $buildCmd;

  # Ensure that the expected test DLL file exists
  if(!(Test-Path -Path $testDllFile)) {
    throw "Unable to find test DLL file - $testDllFile";
  }

  # ---------------------------------------------------- >>
  # Generate coverage report
  $testResultFileName   = Join-Path $testResultsDir "$( $testProject.BaseName )_results.trx";
  $coverageOutputDir    = Join-Path $testCoverageDir ($testProject.BaseName + "\");
  
  # https://docs.microsoft.com/en-us/dotnet/core/tools/dotnet-test
  $dotnetTestTargetArgs = @(
    "test",
    "$( $testProject.FullName )",
    "--logger:trx;LogFileName=$testResultFileName",
    "--configuration $configuration",
    "--no-build", 
    "--no-restore"
  );

  # https://www.jetbrains.com/help/dotcover/dotCover__Console_Runner_Commands.html
  $coverletDotnetArguments = @(
    "$testDllFile",
    "--target `"dotnet`"",
    "--targetargs `"$dotnetTestTargetArgs`"",
    "--output `"$coverageOutputDir`"",
    "--format `"cobertura`"",
    "--format `"opencover`""
  );

  $coverletCmd = "$coverletExe $coverletDotnetArguments";
  Write-Output ("Running coverage :: $coverletCmd");
  Invoke-Expression -Command $coverletCmd;
}
