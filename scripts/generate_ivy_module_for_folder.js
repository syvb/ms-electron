const fs = require('fs');
const xml2js = require('xml2js');

function parseOption (key, defaultValue) {
  for (const arg of process.argv) {
    if (arg.indexOf(`--${key}=`) === 0) {
      return arg.substr(arg.indexOf('=') + 1);
    }
  }
  return defaultValue;
}

function listOfFilesInFolder (folderPath) {
  return fs.readdirSync(folderPath).filter(file => !file.startsWith('.DS'));
}

function parseFileName (file) {
  const index = file.indexOf('.');

  if (index < 0) {
    return parseFileName(`${file}.`);
  }

  return {
    name: file.substr(0, index),
    ext: file.substr(index + 1)
  };
}

function artifactsFromFiles (fileList) {
  return fileList.map(file => ({ $: parseFileName(file) }));
}

const configMap = {
  'dependencies.txt': 'deps',
  'electron.d.ts': 'typings',
  'electron.zip': 'binary',
  'electron-dbg.zip': 'dbg',
  'electron-dsym.zip': 'debug',
  'electron-pdb.zip': 'debug',
  'electron-symbols.zip': 'debug',
  'ffmpeg.zip': 'ffmpeg',
  'node.lib': 'node-lib',
  'node_headers.tar.gz': 'node-headers',
  'third-party_attributions.txt': 'tpn'
};

function addConfigurationsInArtifacts (artifacts) {
  // Resulting configurations should not contain duplicates
  const configurations = new Set();

  for (const artifact of artifacts) {
    const filename = `${artifact.$.name}.${artifact.$.ext}`;
    const conf = configMap[filename] || 'other';

    artifact.$.conf = conf;
    configurations.add(conf);
  }

  return Array.from(configurations).map(name => ({ $: { name } }));
}

const args = {
  teamName: parseOption('team-name', 'electron'),
  moduleName: parseOption('module-name', 'electron-mac-x64'),
  branchName: parseOption('branch-name', 'base-3.1'),
  inputFolder: parseOption('input-folder', 'electron-mac-x64/base-3.1/x64'),
  outputFile: parseOption('output-file', 'ivy-file.xml')
};

const artifacts = artifactsFromFiles(listOfFilesInFolder(args.inputFolder));
const configurations = addConfigurationsInArtifacts(artifacts);

const info = {
  organisation: 'tps',
  'e:team': args.teamName,
  module: args.moduleName,
  branch: args.branchName,
  'e:dname': args.moduleName
};

const root = {
  'ivy-module': {
    $: { version: '2.2', 'xmlns:e': 'http://ant.apache.org/ivy/extra' },
    info: { $: info },
    configurations: { $: { defaultconfmapping: '*' }, conf: configurations },
    publications: { artifact: artifacts }
  }
};

const builder = new xml2js.Builder({ headless: true });
const xml = builder.buildObject(root);

fs.writeFileSync(args.outputFile, xml);
