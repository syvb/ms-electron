## Vocabulary

### action

A set of [Azure Pipelines tasks][] and/or other *actions*.
Actions are pretty generic, and don't make assumptions about a job they're going to be used in,
thus they can't publish build or pipeline artifacts.
But they can have demands for an environment, e.g. a certain platform, or some tool availability.

### job

Basically the same as the [Azure Pipeline job][].
It's a combination of a build environment and a set of *actions* to run in that environment.
It is a job's responsibility to make sure that its environment is capable to execute its actions.

### multi job

A homogeneous set of *jobs*. Typically the same job running on different platforms.

### build

A set of *jobs* and/or *multi jobs*.
It's a top-level entity, the only one that can be run,
and the only one allowed to access [user-defined pipeline variables][].

### executor

Defines and configures execution environment for a *job*.
Typically selects an agent pool and runs a few steps to configure an agent before running the job's steps.

Provides following variables:
- Electron.Executor.GitCacheDirectory
- (optional) SCCACHE_AZURE_BLOB_CONTAINER
- (optional) SCCACHE_AZURE_CONNECTION_STRING
- (optional) SCCACHE_DIR

## Naming conventions

### Jobs and artifacts

CI jobs depend on each other. To express that relationship jobs refer to each other by ids.
Dependent jobs use artifact names to download them from their dependencies.
It's difficult and not always possible to set up all job ids and artifact names on a build level,
naming conventions should solve that problem.

#### job id

-   a job id should match its file name, e.g. a job defined in `'job/do_an_awesome_thing.yml'`
    has to have id of `'do_an_awesome_thing'`.

-   if a job can be executed for various platforms and arches
    its id should include a platform and an arch names, e.g. `'make_it_for_windows_x64'`
    - a build flavour can be a job parameter, but should never be included in a job id

-   a job that produces artifacts should have a "build_" prefix, e.g. `'build_it_mac_x64'`
    - other prefixes can be used if they make more sense than `'build'`

#### artifacts

-   a job artifact's name should be derived from its parent job id:
    - remove the `'build_'` prefix
    - use whitespaces instead of underscores

-   if a job produces multiple artifacts make sure that their names are unique
    and have platform and arch prefixes if it makes sense

It's OK to not follow those rules if it can make everyone's life easier.

## Build agents

All build agent machines are accessible only from the corporate network,
so you have to be connected to the MSFTCORP Wi-Fi, or have VPN enabled.

Use username/password authentication:
- **username**: *[electron-devops-vault][] > Secrets > ElectronBuildagentUsername*
- **password**: *[electron-devops-vault][] > Secrets > ElectronBuildagentPassword*

Azure virtual machines are used to run heavy builds on Linux and Windows.
Use the [Azure portal][] to manage the machines or to get connection details.

To create a new build agent follow the [How To: Create a virtual machine in Azure][] doc.

### Windows agents

There are a number of VM images for create agents from.

#### windows10-vs2017-vm-image

- Windows 10 Pro (version 1809, build 17763.316)
- Visual Studio 2017 Enterprise (15.7.6) with Windows 10 SDK (version 10.0.17134)
- Git 2.21.0
- Python 2.7.15 with PyWin32-224 package (`pip install pywin32`)
- Google Chrome

#### windows10-vs15.9.16-sdk17763-vm-image

- Windows 10 Pro (version 1809, build 17763.737)
- Visual Studio 2017 Enterprise (15.9.16) with Windows 10 SDK (version 10.0.17763)
- Git 2.23.0
- Python 2.7.16 with PyWin32-224 package (`pip install pywin32`)
- Google Chrome

#### windows10-vs15.9.16-sdk18362-vm-image

- Windows 10 Pro (version 1809, build 17763.805)
- Visual Studio 2017 Enterprise (15.9.16) with Windows 10 SDK (version 10.0.18362)
- Git 2.23.0
- Python 2.7.16 with PyWin32-224 package (`pip install pywin32`)
- Google Chrome

#### windows10-2004-vs16.7.2-sdk19041-vm-image

- Windows 10 Pro (version 2004, build 19041.450)
- Visual Studio 2019 Enterprise (16.7.2) with Windows 10 SDK (version 10.0.19041)
- Git 2.28.0
- Python 2.7.18 with PyWin32-224 package (`pip install pywin32`)
- Google Chrome

#### Custom Capabilities

After adding a new build agent add custom [capabilities][] via [agent settings][]:
- "Windows10_SDK: {VERSION_NUMBER}", e.g. "Windows10_SDK: 17134"

### Linux agents

Those use a VM image provided by Azure and are configured on the first boot via a [cloud-init][] script.

### macOS agents

#### Build Configuration

- Mac Pro
- macOS 10.14.x Mojave
- Xcode 9.4.1 (as `/Applications/Xcode_9.4.1.app`)
- Xcode 10.3 (as `/Applications/Xcode_10.3.app`)
- Xcode 11.3.1 (as `/Applications/Xcode_11.3.1.app`)
- Git 2.21.0
- Python 2.7.16 as `python`
- Python 3.7.6 as `python3` (via `brew install python3`)

#### System setup

- Run `/Applications/Python\ 2.7/Install\ Certificates.command` to prevent SSL errors.
- Make [sudo to not require a password][] to make `sudo ...` work in build scripts.
- Make the `elecsa` user to be [auto logged in][].
- Uncheck "Require password ..." in the "Security & Privacy" settings.
- Allow "Screen Sharing" and "Remove Login" in the "Sharing" settings
- Make the machine never go to sleep in the "Energy Saver" settings.
- Turn off the screensaver in the "Desktop & Screen Saver" settings.

On tests agents
  - `defaults write com.apple.CrashReporter DialogType none`
    to suppress the Crash Reporter Dialog.

  - `defaults write com.github.Electron NSQuitAlwaysKeepsWindows -bool false`
    to disable windows Resume.

  - `rm -rf "$HOME/Library/Saved Application State/com.github.Electron.savedState"`
    to [disable the "unexpectedly quit" dialog][].

  - `sudo spctl --master-disable`
    to allow running all apps to run the Azure Pipeline agent.

  - Run the tests locally and 1) allow system notifications from "Electron", 2) allow screen recording for "Terminal".

  - Do not lock the screen.

Build agents have their [screen locked][]. Screen lock AppleScript is compiled
as a proper application so it can be selected in "Login items" in System Settings.

#### Custom Capabilities

After adding a new build agent add custom [capabilities][] via [agent settings][]:
- "Xcode_{VERSION}: true", e.g. "Xcode_11.3.1: true", for all Xcode versions installed

[Azure Pipelines tasks]: https://docs.microsoft.com/en-us/azure/devops/pipelines/process/tasks?view=vsts&tabs=yaml
[Azure Pipeline job]: https://docs.microsoft.com/en-us/azure/devops/pipelines/process/phases?view=vsts&tabs=yaml
[Azure portal]: https://ms.portal.azure.com/
[electron-devops-vault]: https://ms.portal.azure.com/#@microsoft.onmicrosoft.com/resource/subscriptions/1a3c1edb-1a92-4a1e-ab91-251fdc9e6680/resourceGroups/electron-devops-rg/providers/Microsoft.KeyVault/vaults/electron-devops-vault/overview
[user-defined pipeline variables]: https://docs.microsoft.com/en-us/azure/devops/pipelines/process/variables?view=vsts&tabs=yaml%2Cbatch#user-defined-variables
[sudo to not require a password]: https://apple.stackexchange.com/questions/257813/enable-sudo-without-a-password-on-macos
[auto logged in]: https://kb.wisc.edu/page.php?id=39258
[screen locked]: https://apple.stackexchange.com/a/316058
[capabilities]: https://docs.microsoft.com/en-us/azure/devops/pipelines/agents/agents?view=azure-devops#capabilities
[agent settings]: https://domoreexp.visualstudio.com/Teamspace/_settings/agentqueues?queueId=2359&view=jobs
[cloud-init]: https://docs.microsoft.com/en-us/azure/virtual-machines/linux/using-cloud-init
[How To: Create a virtual machine in Azure]: /docs/how_to/create_vm_in_azure.md
[disable the "unexpectedly quit" dialog]: https://github.com/electron/electron/issues/1234
