"""Fail-closed comparison of paired traces; no screenshot or timing sleeps."""
from __future__ import annotations
import argparse, hashlib, html, json
from pathlib import Path


def first_difference(a, b, path='$'):
    if type(a) is not type(b): return {'path':path,'expected':a,'actual':b}
    if isinstance(a, dict):
        for key in sorted(a.keys() | b.keys()):
            if key not in a or key not in b:return {'path':path+'.'+key,'expected':a.get(key),'actual':b.get(key)}
            diff=first_difference(a[key], b[key],path+'.'+key)
            if diff:return diff
    elif isinstance(a,list):
        if len(a)!=len(b):return {'path':path+'.length','expected':len(a),'actual':len(b)}
        for i,(x,y) in enumerate(zip(a,b)):
            diff=first_difference(x,y,f'{path}[{i}]')
            if diff:return diff
    elif a!=b:return {'path':path,'expected':a,'actual':b}
    return None


def changes(records, key):
    out=[];last=object()
    for row in records:
        value=key(row)
        if value!=last:out.append((row['t'],value));last=value
    return out


def semantic_frame(frame):
    ui=frame.get('ui',{})
    window=ui.get('window')
    if window:
        window={**window,'widgets':[{**w,'status':w['status'] & (1|2|4|8|16|32|8192)} for w in window['widgets']]}
    return {'window':window,'combat':ui.get('combat'),'adventure':ui.get('adventure'),
        'game':{k:ui.get('game',{}).get(k) for k in ['day','week','month','player','combat','round']},
        'screen':ui.get('screen'),'layers':ui.get('hd',{}).get('layers')}


def compare(reference, candidate, timing_tolerance=32):
    failures=[]
    def fail(channel,detail,action=None):failures.append({'channel':channel,'action':action,**detail})
    for name,trace in [('reference',reference),('candidate',candidate)]:
        if trace.get('schema')!=1 or trace.get('overflow') or not trace.get('actions'):
            fail('integrity',{'path':name,'error':'Invalid, empty or truncated trace', 'failure':trace.get('failure')})
    if failures:return {'passed':False,'failures':failures}
    # Metadata provenance and the full pre-projection state are mandatory gates.
    for field in ['fixtureSha256','rng','buildConfig','viewport','wasmSha256','observerSha256','settings']:
        if field not in reference or field not in candidate or reference[field]!=candidate[field]:
            fail('initial',{'path':field,'expected':reference.get(field),'actual':candidate.get(field)})
    initial=first_difference(reference.get('initial'),candidate.get('initial'))
    if not reference.get('initial') or initial:fail('initial',initial or {'error':'Missing canonical initial state'})
    if failures:return {'passed':False,'blocked':True,'failures':failures}
    if list(reference['actions'])!=list(candidate['actions']):
        fail('coverage',{'error':'Action coverage differs','expected':list(reference['actions']),'actual':list(candidate['actions'])})
    for action,expected in reference['actions'].items():
        actual=candidate['actions'].get(action)
        if not actual:continue
        for field in ['inputApplied','state','invariants']:
            diff=first_difference(expected.get(field),actual.get(field))
            if diff:fail(field,diff,action)
        for name,trace in [('reference',expected),('candidate',actual)]:
            if not trace.get('frames'):fail('pixels',{'error':name+' has no rendered frames'},action)
            for invariant in trace.get('invariants',[]):
                if not invariant['passed']:fail('invariant',invariant,action)
        # Preserve the entire changed-frame sequence. Dedup only identical frames,
        # not animation frames; compare relative to each causal input action.
        e=changes(expected.get('frames',[]),lambda r:r['tiles'])
        a=changes(actual.get('frames',[]),lambda r:r['tiles'])
        for i in range(max(len(e),len(a))):
            if i>=len(e) or i>=len(a):
                fail('pixels',{'path':f'frame[{i}]','error':'Missing/extra animation frame','expectedCount':len(e),'actualCount':len(a)},action);break
            diff=first_difference(e[i][1],a[i][1])
            if diff:fail('pixels',{'frame':i,**diff},action);break
            delta=(a[i][0]-actual['start'])-(e[i][0]-expected['start'])
            if abs(delta)>timing_tolerance:
                fail('timing',{'frame':i,'deltaMs':delta,'toleranceMs':timing_tolerance},action);break
        e=changes(expected.get('frames',[]),semantic_frame)
        a=changes(actual.get('frames',[]),semantic_frame)
        diff=first_difference([v for _,v in e],[v for _,v in a])
        if diff:fail('interfaceSequence',diff,action)
        # Audio requests carry semantic names; decoded PCM statistics are separate.
        for channel in ['audioEvents','pcm']:
            diff=first_difference(expected.get(channel),actual.get(channel))
            if diff:fail(channel,diff,action)
    return {'passed':not failures,'failures':failures,'firstDivergence':failures[0] if failures else None}


def report(result, path, metadata=None):
    path=Path(path);stats=[]
    for mode in ['legacy','authority']:
        source=path.parent/mode/'trace.json'
        if source.exists():
            trace=json.loads(source.read_text());actions=[v for k,v in trace['actions'].items() if k.endswith('/p0')]
            virtual=sum(v['virtualMs'] for v in actions)/1000;wall=sum(v['wallMs'] for v in actions)/1000
            stats.append(f'<tr><td>{mode}</td><td>{len(actions)}</td><td>{virtual:.2f} с</td><td>{wall:.2f} с</td><td>×{virtual/wall:.1f}</td><td>{trace.get("startupWallSeconds",0):.1f} с</td></tr>' if wall else '')
    links=''.join('<p><a href="'+html.escape(run['report'],quote=True)+'">'+html.escape(name)+'</a> · complete: '+str(run['complete'])+' · parity: '+str(run['parity'])+'</p>' for name,run in (metadata or {}).get('runs',{}).items())
    encoded=json.dumps({'comparison':result,'run':metadata or {}},ensure_ascii=False,indent=2)
    status='PASS' if result['passed'] else 'FAIL'
    rows=[]
    for f in result['failures']:
        detail=html.escape(json.dumps(f,ensure_ascii=False,indent=2))
        rows.append(f'<details><summary>{html.escape(str(f.get("action") or "gate"))} · {html.escape(f["channel"])} · {html.escape(str(f.get("path",f.get("error",""))))[:180]}</summary><pre>{detail}</pre></details>')
    path.write_text(f'''<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>HoMM3 parity — {status}</title><style>body{{font:16px/1.55 system-ui;background:#101621;color:#e6eaf0;max-width:1100px;margin:32px auto;padding:24px}}h1{{color:{'#79dcaa' if result['passed'] else '#ffb180'}}}pre{{background:#1b2534;padding:16px;overflow:auto;font-size:12px;max-height:480px}}a{{color:#9ecbff}}table{{border-collapse:collapse;width:100%;margin:24px 0}}th,td{{text-align:left;border-bottom:1px solid #38465a;padding:10px}}summary{{cursor:pointer;padding:10px 0}}.muted{{color:#a8b5c9}}code{{color:#a5d1ef}}</style><h1>HoMM3 parity · {status}</h1><p>Один сейв · виртуальное время · оба клиента · состояния, кадры и звук.</p><table><tr><th>Режим</th><th>Действий</th><th>Игровое время</th><th>Время прогона</th><th>Ускорение</th><th>Загрузка отдельно</th></tr>{''.join(stats)}</table><p class="muted">Время прогона включает ввод, сбор и передачу трасс обоих клиентов. Загрузка ресурсов и компиляция не входят в ускорение. FAIL означает расхождение либо неполный прогон — см. канал и причину.</p>{links}<h2>{len(rows)} расхождений</h2>{''.join(rows) or '<p>Проверяемые каналы совпали.</p>'}<details><summary>Метаданные и полный JSON</summary><pre>{html.escape(encoded)}</pre></details><p><a href="comparison.json">comparison.json</a> · <a href="legacy/scenario.json">Сценарий replay</a></p></html>''')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('reference',type=Path);p.add_argument('candidate',type=Path);p.add_argument('--out',type=Path,required=True);args=p.parse_args()
    result=compare(json.loads(args.reference.read_text()),json.loads(args.candidate.read_text()));args.out.mkdir(parents=True,exist_ok=True)
    (args.out/'comparison.json').write_text(json.dumps(result,indent=2));report(result,args.out/'index.html');raise SystemExit(0 if result['passed'] else 1)
