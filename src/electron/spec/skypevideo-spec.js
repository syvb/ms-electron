const Module = require('module');
const path = require('path');
Module.globalPaths.push(path.resolve(__dirname, '../../../../electron/spec/node_modules'));

const { expect } = require('chai');

describe('<skypevideo> element', () => {
  it('has HTMLSkypeVideoElement exposed properly', () => {
    expect(HTMLSkypeVideoElement).to.be.a('function');
    expect(HTMLSkypeVideoElement.API_VERSION).to.be.a('number');
    expect(HTMLSkypeVideoElement.prototype.loadSync).to.be.a('function');
  });

  it('works with document.createElement()', () => {
    const skypevideo = document.createElement('skypevideo');
    expect(skypevideo).to.be.instanceOf(HTMLSkypeVideoElement);
  });
});
