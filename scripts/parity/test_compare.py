import copy, unittest
from compare import compare

class Comparator(unittest.TestCase):
    def trace(self):
        return {'schema':1,'wasmSha256':'same-build','observerSha256':'same-observer','settings':{'sound':10},'fixtureSha256':'same','rng':7,'buildConfig':'parity-v1','viewport':[800,600],'initial':{'heroes':[{'id':1,'y':5}]},'actions':{'move':{'start':100,'state':{'y':6,'onMap':True},'invariants':[{'id':'visible','passed':True}],'frames':[{'t':100,'tiles':[1,2]},{'t':116,'tiles':[3,4]}],'audioEvents':[{'name':'horse','t':0}],'pcm':[{'rms':.1}]}}}
    def test_equal(self):self.assertTrue(compare(self.trace(),self.trace())['passed'])
    def test_wrong_save_gates_comparison(self):
        b=self.trace();b['fixtureSha256']='wrong';r=compare(self.trace(),b);self.assertTrue(r['blocked'])
    def test_initial_world_mismatch(self):
        b=self.trace();b['initial']['heroes'][0]['y']=9;self.assertTrue(compare(self.trace(),b)['blocked'])
    def test_seeded_failures(self):
        mutations=[('hero',lambda a:a['state'].update(onMap=False),'state'),('frame',lambda a:a['frames'].pop(),'pixels'),('delay',lambda a:a['frames'][1].update(t=300),'timing'),('sound',lambda a:a['audioEvents'].clear(),'audioEvents'),('pcm',lambda a:a['pcm'][0].update(rms=0),'pcm')]
        for name,mutate,channel in mutations:
            with self.subTest(name=name):
                b=self.trace();mutate(b['actions']['move']);result=compare(self.trace(),b);self.assertFalse(result['passed']);self.assertIn(channel,[f['channel'] for f in result['failures']])
    def test_empty_dropped_and_unrun_never_pass(self):
        for trace in [{},{'schema':1,'actions':{}},{**self.trace(),'overflow':True}]:self.assertFalse(compare(self.trace(),trace)['passed'])

if __name__=='__main__':unittest.main()
