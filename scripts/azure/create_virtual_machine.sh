#!/usr/bin/env bash

# The script uses Azure CLI.
# https://docs.microsoft.com/en-us/cli/azure/
#
# Accepts four positional arguments:
# platform, machine_name, admin_username, admin_password

SCRIPT_DIR=$(dirname "$0")

# A new VM configuration options.
VM_ADMIN_PASSWORD=$4
VM_ADMIN_USERNAME=$3
VM_BOOT_DIAGNOSTICS_STORAGE='electrondevopsrgdiag'
VM_CLOUD_INIT_CONFIG=''  # Will be conditionally defined later.
VM_DISK_TYPE='Premium_LRS'
VM_IMAGE_ID=''  # Will be conditionally defined later.
VM_IMAGE_NAME=''  # Will be conditionally defined later.
VM_LICENSE_TYPE='None'
VM_LOCATION='northeurope'  # Has to be the same as the virtual network location.
VM_NAME=$2
VM_NETWORK_SECURITY_GROUP='electron-devops-nsg'
VM_OS_DISK_SIZE_GB='1024'
VM_PLATFORM=$1
# Explicitly ask for no public IP for the new VM. It's controlled by a policy and we must obey.
VM_PUBLIC_IP=''
VM_RESOURCE_GROUP='electron-devops-rg'
VM_SIZE='Standard_D32s_v4'
# This virtual network has to be used to get access to a VM.
# It is managed externally, we don't have a permission to modify it.
VM_VIRTUAL_NETWORK_NAME='ElectronAtMicrosoft-CORP-NEU-VNET-2804'
VM_VIRTUAL_NETWORK_RESOURCE_GROUP='ERNetwork'
VM_VIRTUAL_NETWORK_SUBNET='Subnet-1'

# Platform-dependent configuration.
case "${VM_PLATFORM}" in
  linux)
    VM_IMAGE_ID='Canonical:UbuntuServer:18.04-LTS:18.04.201906170'
    VM_CLOUD_INIT_CONFIG="${SCRIPT_DIR}/linux_cloud_init.yml"
    ;;
  windows)
    VM_IMAGE_NAME='windows10-2004-vs16.7.2-sdk19041-vm-image'
    VM_LICENSE_TYPE='Windows_Client'
    ;;
  *)
    echo "unexpected platform: '${VM_PLATFORM}'"
    exit 1
    ;;
esac

# Images' names are not unique identifiers, we need proper ids to create a VM.
if [[ -z "$VM_IMAGE_ID" ]]; then
    VM_IMAGE_ID=`az image show --query id --out tsv \
      --resource-group "${VM_RESOURCE_GROUP}" \
      --name "${VM_IMAGE_NAME}"`
fi

# Get an id of the existing virtual network in a different resource group.
# We can't simply pass the virtual network name and subnet to a `az vm create` call
# because they will be treated as a new resource in the current resource group.
# https://docs.microsoft.com/en-us/cli/azure/network/vnet/subnet?view=azure-cli-latest#az-network-vnet-subnet-show
VM_VIRTUAL_NETWORK_SUBNET_ID=`az network vnet subnet show --query id --out tsv \
  --resource-group "${VM_VIRTUAL_NETWORK_RESOURCE_GROUP}" \
  --vnet-name "${VM_VIRTUAL_NETWORK_NAME}" \
  --name "${VM_VIRTUAL_NETWORK_SUBNET}"`

# https://docs.microsoft.com/en-us/cli/azure/vm?view=azure-cli-latest#az_vm_create
az vm create \
  --name "${VM_NAME}" \
  --resource-group "${VM_RESOURCE_GROUP}" \
  --admin-password "${VM_ADMIN_PASSWORD}" \
  --admin-username "${VM_ADMIN_USERNAME}" \
  --boot-diagnostics-storage "${VM_BOOT_DIAGNOSTICS_STORAGE}" \
  --custom-data "${VM_CLOUD_INIT_CONFIG}" \
  --image "${VM_IMAGE_ID}" \
  --license-type "${VM_LICENSE_TYPE}" \
  --location "${VM_LOCATION}" \
  --nsg "${VM_NETWORK_SECURITY_GROUP}" \
  --os-disk-size-gb "${VM_OS_DISK_SIZE_GB}" \
  --public-ip-address "${VM_PUBLIC_IP}" \
  --size "${VM_SIZE}" \
  --storage-sku "${VM_DISK_TYPE}" \
  --subnet "${VM_VIRTUAL_NETWORK_SUBNET_ID}"
