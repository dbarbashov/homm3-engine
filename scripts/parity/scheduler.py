"""Conservative virtual-time barrier for isolated browser seats.
A peer blocked on real transport/assets cannot let another race through the
whole action. Browser clocks remain measured; no captured frames are retimed.
"""
import asyncio


class PresentationScheduler:
    def __init__(self, seats=2):
        self.seats = seats
        self.epoch = 0
        self.active = set()
        self.waiting = {}
        self.packets = {}
        self.receipts = {}
        self.deliveries = {seat: [] for seat in range(seats)}

    def begin(self):
        if self.waiting:
            raise RuntimeError('Previous presentation step has not settled')
        self.epoch += 1
        self.active = set(range(self.seats))
        return self.epoch

    async def arrive(self, seat, event):
        if event['epoch'] != self.epoch:
            return
        if seat not in range(self.seats):
            raise ValueError('Invalid presentation seat')
        if event.get('done'):
            self.active.discard(seat)
            self.release()
            return
        if seat in self.waiting or seat not in self.active:
            raise RuntimeError('Duplicate or inactive presentation step')
        future = asyncio.get_running_loop().create_future()
        self.waiting[seat] = (event['t'], future)
        self.release()
        return await future

    async def network(self, seat, event):
        """Dispatch at the next 8 ms tick, after every peer has left the send tick.
        Same-tick delivery would depend on which browser resumes first.
        """
        if event['kind'] == 'receive':
            key = (event['source'] - 1, seat, event['hash'])
            waiting = self.packets.get(key, [])
            if waiting:
                sent = waiting.pop(0)
                self.deliveries[seat].append((sent, event['id']))
                if not waiting: self.packets.pop(key, None)
            else:
                self.receipts.setdefault(key, []).append(event['id'])
        elif event['kind'] == 'send':
            for target in range(self.seats):
                if target == seat or event['target'] not in (0, target+1): continue
                key = (seat, target, event['hash'])
                received = self.receipts.get(key, [])
                if received:
                    self.deliveries[target].append((event['t'] + 8, received.pop(0)))
                    if not received: self.receipts.pop(key, None)
                else:
                    self.packets.setdefault(key, []).append(event['t'] + 8)
        else: raise ValueError('Unknown transport event')
        self.release()

    def release(self):
        if self.packets or self.receipts or not self.waiting or not self.active.issubset(self.waiting):
            return
        earliest = min(t for t, _ in self.waiting.values())
        for seat, (t, future) in list(self.waiting.items()):
            if t == earliest:
                del self.waiting[seat]
                if not future.done():
                    deliver=[packet for sent, packet in self.deliveries[seat] if sent<=t]
                    self.deliveries[seat]=[(sent,packet) for sent,packet in self.deliveries[seat] if sent>t]
                    future.set_result({'deliver':deliver})

    def close(self):
        self.packets.clear(); self.receipts.clear()
        for _, future in self.waiting.values():
            future.cancel()
        self.waiting.clear()
        self.active.clear()
