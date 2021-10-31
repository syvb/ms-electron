# This script is supposed to be executed on an Windows virtual machine in Azure.

# Azure CLI passes anonymous arguments as named "arg1", "arg2", etc.
# Positional arguments are used in the Azure CLI invocation
# to keep it cross-platform and support both PowerShell and Unix Shell scripts.
param (
    [Parameter(Mandatory=$true)][string]$arg1,  # the agent name
    [Parameter(Mandatory=$true)][string]$arg2,  # a local user name
    [Parameter(Mandatory=$true)][string]$arg3,  # a local user password
    [Parameter(Mandatory=$true)][string]$arg4  # a personal token to access the server
)

# Create a folder for the agent.
$agentFolder = 'C:\agent'
if (Test-Path -Path $agentFolder)
{
    Write-Host -Object "The agent folder already exists, terminating the script."
    exit 1
}
New-Item -ItemType directory -Path $agentFolder

# Download an agent archive.
$agentVersion = '2.174.1'
$agentDownloadUrl = "https://vstsagentpackage.azureedge.net/agent/$agentVersion/vsts-agent-win-x64-$agentVersion.zip"
$agentDownloadLocation = "$env:TEMP\agent.zip"
Invoke-WebRequest -Uri $agentDownloadUrl -OutFile $agentDownloadLocation

# Unpack the agent files.
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory("$agentDownloadLocation", "$agentFolder")

# Configure and start the agent.
# https://docs.microsoft.com/en-us/azure/devops/pipelines/agents/v2-windows?view=azure-devops#unattended-config
$localUserName = $arg2
$localUserPassword = $arg3
$serverUrl = 'https://domoreexp.visualstudio.com'
$serverAgentName = $arg1
$serverAgentPool = 'Electron Windows'
$serverAgentToken = $arg4

Set-Location $agentFolder
$scriptPath = ".\config.cmd"
$cmdArguments = ('/c', $scriptPath, '--unattended', `
    '--url', $serverUrl, `
    '--auth', 'pat', `
    '--token', $serverAgentToken, `
    '--pool', "`"$serverAgentPool`"", `
    '--agent', "`"$serverAgentName`"", `
    '--runAsAutoLogon', `
    '--windowsLogonAccount', "`"$localUserName`"", `
    '--windowsLogonPassword', "`"$localUserPassword`""
)

$process = Start-Process -FilePath cmd.exe -ArgumentList $cmdArguments -Wait -PassThru
$exitCode = $process.ExitCode
if ($exitCode -ne 0)
{
    Write-Host -Object "Something went wrong."
    exit $exitCode
}
