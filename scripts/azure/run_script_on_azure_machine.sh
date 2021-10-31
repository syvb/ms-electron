#!/usr/bin/env bash

# The script uses Azure CLI.
# https://docs.microsoft.com/en-us/cli/azure/
#
# Accepts the following arguments:
# machine_name, script_name, ...script_args

VM_NAME=$1
SCRIPT_NAME=$2
shift; shift;
SCRIPT_ARGS=$@

COMMAND_ID=''  # Will be conditionally defined later.
COMMAND_SCRIPT_PATH=''  # Will be conditionally defined later.
SCRIPT_DIR=$(dirname "$0")
VM_RESOURCE_GROUP='electron-devops-rg'

# Get a VM OS type by its name. The command will return 'Linux' or 'Windows'.
VM_PLATFORM=`az vm show --out tsv \
  --name ${VM_NAME} \
  --resource-group ${VM_RESOURCE_GROUP} \
  --query 'storageProfile.osDisk.osType'`

# Platform-dependent configuration.
case "${VM_PLATFORM}" in
  Linux)
    COMMAND_ID='RunShellScript'
    COMMAND_SCRIPT_PATH="${SCRIPT_DIR}/${SCRIPT_NAME}.sh"
    ;;
  Windows)
    COMMAND_ID='RunPowerShellScript'
    COMMAND_SCRIPT_PATH="${SCRIPT_DIR}/${SCRIPT_NAME}.ps1"
    ;;
  *)
    echo "unexpected platform: '${VM_PLATFORM}'"
    exit 1
    ;;
esac

# https://docs.microsoft.com/en-us/cli/azure/vm/run-command?view=azure-cli-latest#az-vm-run-command-invoke
COMMAND_TEXT="az vm run-command invoke \
  --command-id ${COMMAND_ID} \
  --name ${VM_NAME}\
  --parameters ${SCRIPT_ARGS} \
  --resource-group ${VM_RESOURCE_GROUP} \
  --scripts @${COMMAND_SCRIPT_PATH}"
echo ${COMMAND_TEXT}

COMMAND_OUTPUT=`${COMMAND_TEXT}`
echo ${COMMAND_OUTPUT}
