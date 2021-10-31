#!/usr/bin/env bash

# This script is supposed to be executed on a Linux virtual machine in Azure.
#
# Accepts the following positional arguments:
# agent_name, user_name, user_password, personal_access_token

# (alkuzmin): Doesn't work.
# It looks like parameters are not passed to the script.
exit 1

USER_NAME=$2
sudo -i -u ${USER_NAME} bash << EOF

# Create a folder for the agent.
AGENT_HOME="${HOME}/agent"

if [[ -d "${AGENT_HOME}" ]]; then
  echo "The agent folder already exists, terminating the script."
  exit 1
fi
mkdir "${AGENT_HOME}"
cd "${AGENT_HOME}"

# Download an agent archive.
AGENT_VERSION='2.174.1'
AGENT_DOWNLOAD_URL="https://vstsagentpackage.azureedge.net/agent/${AGENT_VERSION}/vsts-agent-linux-x64-${AGENT_VERSION}.tar.gz"
AGENT_DOWNLOAD_LOCATION="${AGENT_HOME}/agent-${AGENT_VERSION}.tar.gz"
curl "${AGENT_DOWNLOAD_URL}" > "${AGENT_DOWNLOAD_LOCATION}"

# Unpack the agent files.
tar zxvf "${AGENT_DOWNLOAD_LOCATION}"

# Configure the agent.
# https://docs.microsoft.com/en-us/azure/devops/pipelines/agents/v2-windows?view=azure-devops#unattended-config
SERVER_URL='https://domoreexp.visualstudio.com'
SERVER_AGENT_NAME=$1
SERVER_AGENT_POOL='Electron Linux'
SERVER_AGENT_TOKEN=$4

./config.sh --unattended \
  --acceptTeeEula \
  --url "${SERVER_URL}" \
  --auth pat \
  --token "${SERVER_AGENT_TOKEN}" \
  --pool "${SERVER_AGENT_POOL}" \
  --agent "${SERVER_AGENT_NAME}"

# Run as a systemd service.
sudo ./svc.sh install
sudo ./svc.sh start

EOF
