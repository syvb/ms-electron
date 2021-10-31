/*
 * Downloads impersonation token to specified destination directory.
 * The token is needed for publishing to Skype IVY from location outside Skype TPS Azure Pipelines.
 * Resulting token files should be placed to ${basedir} of the build.xml file used for publishing.
 *
 * Related links:
 * Skype IVY location: https://skype.pkgs.visualstudio.com/_packaging/tps/ivy/v1/tps/
 * API documentation: https://skype.visualstudio.com/ES/ES%20Team/_git/internal_tps_build?path=%2FREADME.md&version=GBmaster
 * Skype TPS Azure DevOps: https://skype.visualstudio.com/TPS
 * skype-ant: https://skype.pkgs.visualstudio.com/_packaging/csc/ivy/v1/skype/tools_build/skype-ant/master/anyos-anycpu/
 *
 * Parameters (with examples):
 * ===========================
 *
 * registration-id
 * ---------------
 * https://skype.visualstudio.com/SCC/_componentGovernance/client-shared_electron_electron-buildscript/9652577
 * ID of the registration in Skype Azure DevOps component governance. Any successful registration can be used.
 *
 * repository-id
 * -------------
 * The id of the repository with the code used for registrations. In this case it is:
 * https://skype.visualstudio.com/SCC/_git/client-shared_electron_electron-buildscript
 * which has ID 47917. This number is can be obtained from Azure Pipelines web API.
 *
 * reference-name
 * --------------
 * https://skype.visualstudio.com/SCC/_componentGovernance/client-shared_electron_electron-buildscript/9652577
 * See the title in the registration. Tpsfeed will not alow to create token if the name is wrong.
 *
 * reference-version
 * -----------------
 * https://skype.visualstudio.com/SCC/_componentGovernance/client-shared_electron_electron-buildscript/9652577
 * See the git hash in parenthesis. It must be equal or the token will not work.
 *
 * user, password
 * --------------
 * Should be username and Personal Access Token which has access to Skype TPS Azure Pipelines and Skype IVY
 *
 * publish-build-name
 * ------------------
 * It seems this is actually not so importtant. Use either module name or organization (for example 'electron') name.
 *
 * publishVersion
 * --------------
 * This should be the revision id (or buildNumber). Skype IVY will decline uploading anything with different version.
 */

const fs = require('fs');
const path = require('path');
const https = require('https');

function parseOption (key, defaultValue) {
  for (const arg of process.argv) {
    if (arg.indexOf(`--${key}=`) === 0) {
      return arg.substr(arg.indexOf('=') + 1);
    }
  }
  return defaultValue;
}

function generatePostData ({
  publishBuildName,
  publishVersion,
  registrationId,
  repositoryId,
  referenceName,
  referenceVersion
}) {
  return {
    references: [{
      registrationId: registrationId,
      name: referenceName,
      repositoryId: repositoryId,
      project: 'scc',
      version: referenceVersion
    }],
    builds: [{
      version: publishVersion,
      protocol: 'ivy',
      name: publishBuildName
    }]
  };
}

function postRequestOptions (hostName, userName, password, postData) {
  return {
    protocol: 'https:',
    hostname: hostName,
    port: 443,
    path: '/api/create',
    method: 'POST',
    auth: `${userName}:${password}`,
    headers: {
      'Content-Type': 'application/json',
      'Content-Length': Buffer.byteLength(postData)
    }
  };
}

function requestImpersonationToken (hostName, userName, password, postData) {
  return new Promise((resolve, reject) => {
    const options = postRequestOptions(hostName, userName, password, postData);

    const request = https.request(options, (response) => {
      let receivedData = '';
      response.setEncoding('utf8');
      response.on('data', (chunk) => { receivedData += chunk; });
      response.on('end', () => { resolve(receivedData); });
    });

    request.on('error', (e) => { reject(e); });
    request.end(postData);
  });
}

async function main () {
  const args = {
    tpsfeedUsername: parseOption('user', ''),
    tpsfeedPassword: parseOption('password', ''),
    destination: parseOption('destination', '.'),
    publishBuildName: parseOption('publish-build-name', 'Microsoft.Skype.TPS.Electron'),
    publishVersion: parseOption('publish-version', '10101010'),
    registrationId: parseOption('registration-id', '9652577'),
    repositoryId: parseOption('repository-id', '47917'),
    referenceName: parseOption('reference-name', 'electron/libchromiumcontent'),
    referenceVersion: parseOption('reference-version', '82816abaad3930c3702cf23489f0506ba13b1529')
  };

  const postData = JSON.stringify(generatePostData(args));

  const hostName = 'tpsfeed.azurewebsites.net';
  const userName = args.tpsfeedUsername;
  const password = args.tpsfeedPassword;

  const responseJSON = await requestImpersonationToken(hostName, userName, password, postData);

  const response = JSON.parse(responseJSON);

  const impersonationContent = Buffer.from(response.protocols.Ivy.content, 'base64').toString('ascii');
  fs.writeFileSync(path.join(args.destination, response.protocols.Ivy.name), impersonationContent);

  const tokenContent = `${response.scope} ${response.token}`;
  fs.writeFileSync(path.join(args.destination, 'tps-auth.token'), tokenContent);
}

(async () => {
  try {
    await main();
  } catch (e) {
    console.error(`${e}`);
  }
})();
