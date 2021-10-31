#!/usr/bin/env bash

# The script uses Azure CLI.
# https://docs.microsoft.com/en-us/cli/azure/
#
# Accepts one positional arguments:
# machine_name

VM_NAME=$1
VM_RESOURCE_GROUP='electron-devops-rg'

# https://docs.microsoft.com/en-us/cli/azure/vm?view=azure-cli-latest#az-vm-deallocate
az vm deallocate \
  --name "${VM_NAME}" \
  --resource-group "${VM_RESOURCE_GROUP}"
