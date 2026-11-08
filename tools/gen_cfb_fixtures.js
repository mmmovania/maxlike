// Test files for maxlike's CFB reader, written by an independent implementation
// (SheetJS cfb 1.2.2). Each stream's byte i is (i * mul + add) & 0xFF.
const CFB = require('cfb');
function pattern(n, mul, add) { const b = Buffer.alloc(n); for (let i = 0; i < n; i++) b[i] = (i * mul + add) & 0xFF; return b; }
function make(file, streams) {
  const c = CFB.utils.cfb_new();
  for (const [name, n, mul, add] of streams) CFB.utils.cfb_add(c, '/' + name, pattern(n, mul, add));
  CFB.utils.cfb_del(c, '/\u0001Sh33tJ5');   // SheetJS's own placeholder stream, if present
  require('fs').writeFileSync(file, CFB.write(c, {type: 'buffer', fileType: 'cfb'}));
  const back = CFB.read(require('fs').readFileSync(file), {type: 'buffer'});
  console.log(file, back.FullPaths.join(' | '));
}
// Run from the repository root: npm install cfb@1.2.2, then node tools/gen_cfb_fixtures.js
const out = 'tests/data/';
make(out + 'cfb-small.cfb', [['Scene', 100, 7, 3], ['ClassDirectory3', 40, 11, 5], ['\u0005SummaryInformation', 64, 13, 1]]);
make(out + 'cfb-large.cfb', [['Scene', 10000, 7, 3], ['DllDirectory', 300, 3, 9], ['Config', 5000, 5, 2]]);
