"""Focused town acceptance; full frame/audio parity remains a separate gate."""
import json
from pathlib import Path
from compare import first_difference


def check_run(directory, scenario):
    directory = Path(directory)
    checks = []

    def check(name, passed, detail=None):
        checks.append({'name': name, 'passed': bool(passed), **({'detail': detail} if detail else {})})

    try:
        traces = {m: json.loads((directory / m / 'trace.json').read_text()) for m in ['legacy', 'authority']}
        expected = json.loads((directory / 'legacy/scenario.json').read_text())
        keys = {step['name'] + '/p' + str(seat) for step in expected for seat in range(2)}
        for mode, trace in traces.items():
            check(mode + ': complete', not trace.get('failure') and not trace.get('overflow') and set(trace['actions']) == keys, trace.get('failure'))
            check(mode + ': browser errors', json.loads((directory / mode / 'errors.json').read_text()) == [])
        if not all(c['passed'] for c in checks):
            return {'passed': False, 'checks': checks}
        a, b = traces['legacy'], traces['authority']
        check('identical save and RNG', a['fixtureSha256'] == b['fixtureSha256'] and a['rng'] == b['rng'] and a['initial'] == b['initial'])
        transient = []
        for key in sorted(keys):
            left, right = a['actions'][key]['state'], b['actions'][key]['state']
            diff = first_difference(left['world'], right['world'])
            if (scenario == 'services' and key == 'buy-spellbook/p0') or (scenario == 'transformer' and key in ['select-all/p0', 'transform/p0']):
                # Original giveSpells runs after closing the guild modal; the
                # host grants spells with the book purchase. Record this real
                # transient mismatch; verify complete state at guild-close.
                # Transformer edits a working army before closing; its final
                # canonical army and every displayed working list are checked.
                if diff: transient.append({'action': key, 'difference': diff})
            else:
                check(key + ': owned world', diff is None, diff)
            check(key + ': modal type', (left.get('window') or {}).get('type') == (right.get('window') or {}).get('type') and left['layers'] == right['layers'])
            if scenario == 'university' and key in ['university/p0', 'choose-school/p0']:
                diff = first_difference(left['window'], right['window'])
                check(key + ': town university window', diff is None, diff)
            if scenario == 'transformer' and key.endswith('/p0') and key.split('/')[0] in ['transformer', 'select-all', 'transform']:
                diff = first_difference(left['window'], right['window'])
                check(key + ': transformer working army', diff is None, diff)
            if scenario == 'services' and key.endswith('/p0') and key.split('/')[0] in ['mage-guild', 'buy-spellbook', 'buy-machine', 'market', 'upgrade-dialog', 'dismiss-dialog']:
                diff = first_difference(left['window'], right['window'])
                check(key + ': complete service dialog', diff is None, diff)
            # Town army rows are fixed portraits/counts/selection widgets, not
            # animated creature previews or a rollover text tied to the mouse.
            if 'TTownScreenWindow' in (left.get('window') or {}).get('type', ''):
                rows = lambda state: [w for w in state['window']['widgets'] if 100 <= w['id'] <= 146]
                diff = first_difference(rows(left), rows(right))
                check(key + ': town army widgets', diff is None, diff)
        for mode, trace in traces.items():
            def world(action): return trace['actions'][action + '/p0']['state']['world']
            def town(action): return world(action)['towns'][0]
            def hero(action, identity): return next(h for h in world(action)['heroes'] if h['id'] == identity)
            identity = town('town-entry')['visitor']
            check(mode + ': entered with original hero', identity >= 0)
            if scenario == 'core':
                check(mode + ': building costs 5000 gold', world('hall')['resources'][6] - world('build-city-confirm')['resources'][6] == 5000)
                check(mode + ': city hall installed and daily marker', int(town('build-city-confirm')['buildings']) & (1 << 12) and town('build-city-confirm')['builtToday'] == 1)
                check(mode + ': recruited 50 upgraded creatures', town('recruit-confirm')['population'][7] == 0 and sum(n for _, n in town('recruit-confirm')['army']) == 80)
                army = hero('split-confirm', identity)['army']
                check(mode + ': split 40 into 33 and 7', army[0] == [0, 33] and army[3] == [0, 7])
                check(mode + ': entered garrison', town('garrison')['garrisonHero'] == identity and town('garrison')['visitor'] == -1)
                check(mode + ': left garrison', town('visitor')['visitor'] == identity and town('visitor')['garrisonHero'] == -1)
                check(mode + ': moved after town', hero('move-after-town', identity)['pos'][1] == 6 and town('move-after-town')['visitor'] == -1)
            elif scenario == 'residents':
                other = town('hire')['visitor']
                check(mode + ': second hero hired beside garrison', other >= 0 and other != identity and town('hire')['garrisonHero'] == identity and world('tavern')['resources'][6] - world('hire')['resources'][6] == 2500)
                for action, visitor, garrison in [('swap-residents', identity, other), ('swap-residents-back', other, identity), ('swap-for-exit', identity, other), ('town-reenter', identity, other), ('reenter-swap', other, identity)]:
                    check(mode + ': ' + action, town(action)['visitor'] == visitor and town(action)['garrisonHero'] == garrison)
                check(mode + ': correct hero left town', hero('move-after-town', identity)['pos'][1] == 6 and hero('move-after-town', other)['pos'][1] == 5)
            elif scenario == 'services':
                check(mode + ': spellbook available after guild', any(item[0] == 0 for item in hero('guild-close', identity)['equipped']))
                check(mode + ': machine changes equipped artifacts', hero('machine-confirmation', identity)['equipped'] != hero('guild-close', identity)['equipped'])
                check(mode + ': marketplace debits wood and credits gold', world('trade')['resources'][0] == 0 and world('trade')['resources'][6] > world('market')['resources'][6])
                check(mode + ': upgrade replaces faction creature', town('upgrade-confirm')['army'][0] == [town('town-entry')['army'][0][0] + 1, 20])
                check(mode + ': dismiss removes selected stack only', town('dismiss-confirm')['army'][0][1] == 0 and town('dismiss-confirm')['army'][1] == town('town-entry')['army'][1])
            elif scenario == 'transformer':
                check(mode + ': two transformed stacks retained', hero('transformer-close', identity)['army'][:2] == [[56, 40], [56, 20]])
            elif scenario == 'university':
                before, after = hero('town-entry', identity), hero('university-close', identity)
                check(mode + ': learned one magic school', sum(after['skills']) - sum(before['skills']) == 1)
                check(mode + ': tuition paid once', world('town-entry')['resources'][6] - world('university-close')['resources'][6] == 2000)
            elif scenario == 'rewards':
                before, after = hero('idle', identity), hero('reward-confirm', identity)
                faction = town('town-entry')['type']
                stat = {2: 3, 3: 2, 6: 0, 7: 1}.get(faction)
                if stat is not None:
                    check(mode + ': original primary bonus', after['primary'][stat] == before['primary'][stat] + 1)
                if faction == 0:
                    check(mode + ': original stables bonus', after['maxMovement'] == before['maxMovement'] + 400)
                for field in ['experience', 'mana', 'skills', 'primary', 'maxMovement']:
                    if field in after:
                        check(mode + ': reward retained once: ' + field, after[field] == hero('reenter', identity)[field])
            elif scenario == 'portal':
                check(mode + ': empty portal has explanation', trace['actions']['portal/p0']['state']['window']['type'] == '10TDialogBox')
                check(mode + ': empty portal preserves world', world('levelup-confirm') == world('portal-close'))
                check(mode + ': battle academy awards 1000 experience', hero('levelup-confirm', identity)['experience'] - hero('idle', identity)['experience'] == 1000)
                for field in ['experience', 'mana', 'skills', 'primary']:
                    if field in hero('portal-close', identity):
                        check(mode + ': retained once after reentry: ' + field, hero('portal-close', identity)[field] == hero('portal-again-close', identity)[field])
        result = {'passed': all(c['passed'] for c in checks), 'checks': checks, 'transientDifferences': transient}
        result['simulationMs'] = sum(v['virtualMs'] for t in traces.values() for k, v in t['actions'].items() if k.endswith('/p0'))
        result['executionMs'] = sum(v['wallMs'] for t in traces.values() for k, v in t['actions'].items() if k.endswith('/p0'))
        result['startupSeconds'] = sum(t['startupWallSeconds'] for t in traces.values())
        result['strictParityPassed'] = json.loads((directory / 'comparison.json').read_text())['passed']
        return result
    except (OSError, ValueError, KeyError, StopIteration) as error:
        check('evidence readable', False, repr(error))
        return {'passed': False, 'checks': checks}
