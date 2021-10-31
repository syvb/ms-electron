const Module = require('module');
const path = require('path');
Module.globalPaths.push(path.resolve(__dirname, '../../../../electron/spec/node_modules'));

const { expect } = require('chai');

describe('Dummy remote spec', () => {
  it('works', () => {
    expect(true).to.be.ok;
  });
});
