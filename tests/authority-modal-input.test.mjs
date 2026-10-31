import {test} from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {spawnSync} from 'node:child_process';
const root=path.resolve(path.dirname(new URL(import.meta.url).pathname),'..');
function compileAndRun(source,fullGame){
  const directory=fs.mkdtempSync(path.join(os.tmpdir(),'authority-input-'));
  try{
    const file=path.join(directory,'probe.cpp'),binary=path.join(directory,'probe');fs.writeFileSync(file,source);
    const built=spawnSync(process.env.CXX||'clang++',['-std=c++14',...(fullGame?['-DHOMM3_FULL_GAME']:[]),'-I'+path.join(root,'port/compat'),file,'-o',binary],{encoding:'utf8'});
    assert.equal(built.status,0,built.stderr);const ran=spawnSync(binary,[],{encoding:'utf8'});assert.equal(ran.status,0,ran.stderr);
  }finally{fs.rmSync(directory,{recursive:true,force:true});}
}
test('awaited commands allow input only while an authoritative original modal owns it',()=>{
  compileAndRun(`#include "fast_game.h"
#include <cassert>
bool modal=false;
namespace Port { namespace AuthorityClient { bool modalInput(){return modal;} } }
int main(){
  auto& state=Port::Fast::state();state.active=true;state.waiting=true;
  assert(Port::Fast::blocksInput());
  modal=true;assert(!Port::Fast::blocksInput());
  state.halted=true;assert(Port::Fast::blocksInput());state.halted=false;
  state.running=true;assert(Port::Fast::blocksInput());state.running=false;
  modal=false;state.waiting=false;assert(!Port::Fast::blocksInput());
}`,true);
});
test('the separately linked menu guard needs no authoritative client symbol',()=>{
  compileAndRun(`#include "fast_game.h"
#include <cassert>
int main(){auto& state=Port::Fast::state();state.active=true;state.waiting=true;assert(Port::Fast::blocksInput());state.waiting=false;assert(!Port::Fast::blocksInput());}`,false);
});
