"""Compare the native worker RNG with the actual pinned Emscripten libc.

No harness RNG is linked. Two translation units check shared srand/rand state,
including unsigned wrap at seed zero and the unseeded initial sequence.
"""
import argparse
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--out',type=Path,default=ROOT/'test-results/server-rng')
p.add_argument('--emcc',type=Path,default=ROOT/'.local/emsdk/upstream/emscripten/em++')
a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
(out/'seed.cpp').write_text('#include <cstdlib>\nvoid reseed(unsigned value){srand(value);}\n')
(out/'probe.cpp').write_text('''#include <cstdlib>
#include <cstdint>
#include <cstdio>
void reseed(unsigned);
int main(){
    const unsigned seeds[]={0,1,42,1975432,2147483647u,2147483648u,4294967294u,4294967295u};
    for(int trial=-1;trial<8;++trial){
        if(trial>=0)reseed(seeds[trial]);
        std::uint32_t hash=2166136261u;
        std::printf("%d:",trial);
        for(int i=0;i<100000;++i){unsigned value=rand();if(i<8)std::printf("%u,",value);hash=(hash^value)*16777619u;}
        std::printf("%u\\n",hash);
    }
}
''')
source=[str(out/'probe.cpp'),str(out/'seed.cpp')]
subprocess.run(['clang++','-std=c++14','-O2','-DHOMM3_SERVER','-include',str(ROOT/'port/compat/prefix.h'),*source,'-o',str(out/'native')],check=True)
subprocess.run([str(a.emcc),'-std=c++14','-O2','-sENVIRONMENT=node',*source,'-o',str(out/'wasm.cjs')],check=True)
native=subprocess.check_output([str(out/'native')],text=True)
wasm=subprocess.check_output(['node',str(out/'wasm.cjs')],text=True)
(out/'native.txt').write_text(native);(out/'wasm.txt').write_text(wasm)
result={'passed':native==wasm,'sequences':9,'drawsPerSequence':100000,'totalDraws':900000,'includesUnseeded':True,'includesSeedZeroWrap':True,'harness':False}
(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));assert result['passed'],'Native RNG differs from Emscripten libc'
