# How To: Create a virtual machine in Azure

## TL;DR
Use a [CI job][] to create a VM then use credentials from the [electron-devops-vault][] to access the new VM.  
Once you no longer need the VM please delete it.

## Overview

The team owns an [Azure subscription][] under which we can create all kind of Azure resources.  
We use it to publish Electron distribution, and to create virtual machines to run CI builds,  
but it also can be used to create VMs for experiments or to work on platform-specific bugs or features.  
*If you need a VM for your personal needs please use your personal subscription.*

## So... how do I create a VM?

There's [a script](/scripts/azure/create_virtual_machine.sh) to do it.
Check its contents to find out what arguments it accepts.  
It uses [Azure CLI][] so you have to have it installed, and be able to login to the [Azure Portal][].  
To avoid the hassle of setting everything up locally a [CI job][] has been created to run that script.

## Why can't I create a VM myself on the Azure Portal?

Of course you can.  
There are a few tricky parts though, the script handles them all.

[Azure CLI]: https://docs.microsoft.com/en-us/cli/azure/
[Azure Portal]: http://ms.portal.azure.com/
[Azure subscription]: https://ms.portal.azure.com/#@microsoft.onmicrosoft.com/resource/subscriptions/1a3c1edb-1a92-4a1e-ab91-251fdc9e6680/overview
[CI job]: https://domoreexp.visualstudio.com/Teamspace/_build?definitionId=2855
[electron-devops-vault]: https://ms.portal.azure.com/#@microsoft.onmicrosoft.com/resource/subscriptions/1a3c1edb-1a92-4a1e-ab91-251fdc9e6680/resourceGroups/electron-devops-rg/providers/Microsoft.KeyVault/vaults/electron-devops-vault/overview
