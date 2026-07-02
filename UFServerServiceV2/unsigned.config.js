// Unsigned build config: the package.json "build" config minus Azure
// Trusted Signing (electron-builder's schema no longer accepts the
// -c.win.azureSignOptions=null CLI override).
const base = require('./package.json').build;
const win = { ...base.win };
delete win.azureSignOptions;
module.exports = { ...base, win };
