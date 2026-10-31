import {test} from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {spawnSync} from 'node:child_process';

const root = path.resolve(path.dirname(new URL(import.meta.url).pathname), '..');
test('artifact component iteration works with standard algorithms across bitset words', () => {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'artifact-iterator-'));
  try {
    const source = path.join(directory, 'probe.cpp');
    const binary = path.join(directory, 'probe');
    fs.writeFileSync(source, `#include "const_bitset_iterator.h"
#include <algorithm>
#include <cassert>
#include <type_traits>
int main() {
  using Iterator = TConstBitsetIterator<144>;
  static_assert(std::is_same<std::iterator_traits<Iterator>::value_type, bool>::value, "traits");
  std::bitset<144> bits;
  Iterator end(bits, 144);
  assert(!(std::find_if(Iterator(bits, 0), end, TBitIsSet()) != end));
  for (auto index : {0, 63, 64, 143}) bits.set(index);
  Iterator current(bits, 0);
  for (auto expected : {0, 63, 64, 143}) {
    current = std::find_if(current, end, TBitIsSet());
    assert(current != end && current.position() == expected);
    ++current;
  }
  assert(!(current != end));
}
`);
    const built = spawnSync(process.env.CXX || 'clang++', ['-std=c++14', '-I' + path.join(root, 'include'), source, '-o', binary], {encoding: 'utf8'});
    assert.equal(built.status, 0, built.stderr);
    const run = spawnSync(binary, [], {encoding: 'utf8'});
    assert.equal(run.status, 0, run.stderr);
  } finally {
    fs.rmSync(directory, {recursive: true, force: true});
  }
});
