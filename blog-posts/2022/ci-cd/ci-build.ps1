<#(Rn.BuildScriptHelper){
  "version": "1.0.107"
}#>

param (
  [Parameter(Mandatory=$true)]
  [string] $project,

  [Parameter(Mandatory=$false)]
  [string] $rootDir = $PSScriptRoot,

  [Parameter(Mandatory=$false)]
  [string] $configuration = "Release"
)

$rootDir          = [IO.Path]::GetFullPath((Join-Path $rootDir "\..\"));
$sourceDir        = Join-Path $rootDir "src\";
$projectRootDir   = Join-Path $sourceDir ($project + "\");
$artifactDir      = Join-Path $rootDir "artifacts\";
$publisRoot       = Join-Path $artifactDir "publish\";
$publishDir       = Join-Path $publisRoot ($project + "\");

# =============================================================================
# Info Dumping
# =============================================================================
#
Write-Output ("=============================================================");
Write-Output ("= ci-build.ps1 :: information");
Write-Output ("=============================================================");
Write-Output ("= rootDir        : $rootDir");
Write-Output ("= sourceDir      : $sourceDir");
Write-Output ("= projectRootDir : $projectRootDir");
Write-Output ("= artifactDir    : $artifactDir");
Write-Output ("= publisRoot     : $publisRoot");
Write-Output ("= publishDir     : $publishDir");
Write-Output ("=============================================================");


# =============================================================================
# Build project
# =============================================================================
#
$buildCmd = "dotnet build `"$projectRootDir`" --configuration $configuration";
Write-Output ("(INFO) Building project :: $buildCmd");
Invoke-Expression $buildCmd;

$publishCmd = "dotnet publish `"$projectRootDir`" --configuration $Configuration --no-restore --output `"$publishDir`"";
Write-Output ("(INFO) Publishing project :: $publishCmd");
Invoke-Expression $publishCmd;
