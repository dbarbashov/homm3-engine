import asyncio
import unittest
from scheduler import PresentationScheduler


class SchedulerTests(unittest.IsolatedAsyncioTestCase):
    async def test_transport_blocks_peer_and_earliest_clock_runs_first(self):
        clock = PresentationScheduler()
        epoch = clock.begin()
        def step(seat, t):
            return asyncio.create_task(clock.arrive(seat, {'epoch': epoch, 't': t}))
        ahead = step(1, 16)
        await asyncio.sleep(0)
        self.assertFalse(ahead.done())
        behind = step(0, 8)
        await behind
        self.assertFalse(ahead.done())
        caught_up = step(0, 16)
        await asyncio.gather(caught_up, ahead)
        final = step(1, 24)
        await asyncio.sleep(0)
        self.assertFalse(final.done())
        await clock.arrive(0, {'epoch': epoch, 't': 16, 'done': True})
        await final
        await clock.arrive(1, {'epoch': epoch, 't': 24, 'done': True})
        self.assertEqual(clock.begin(), epoch + 1)
        await clock.arrive(0, {'epoch': epoch, 't': 16, 'done': True})
        self.assertEqual(clock.active, {0, 1})
        clock.close()

    async def test_overlapping_epochs_rejected_and_close_cancels_wait(self):
        clock = PresentationScheduler()
        epoch = clock.begin()
        wait = asyncio.create_task(clock.arrive(0, {'epoch': epoch, 't': 8}))
        await asyncio.sleep(0)
        with self.assertRaises(RuntimeError):
            clock.begin()
        clock.close()
        with self.assertRaises(asyncio.CancelledError):
            await wait

    async def test_binary_delivery_is_scheduled_after_real_receipt(self):
        clock=PresentationScheduler(2);epoch=clock.begin()
        await clock.network(0,{'kind':'send','target':2,'hash':'one','t':8})
        sender=asyncio.create_task(clock.arrive(0,{'epoch':epoch,'t':16}))
        receiver=asyncio.create_task(clock.arrive(1,{'epoch':epoch,'t':8}))
        await asyncio.sleep(0)
        self.assertFalse(sender.done());self.assertFalse(receiver.done())
        await clock.network(1,{'kind':'receive','source':1,'hash':'one','id':10})
        self.assertEqual(await receiver,{'deliver':[]})
        self.assertFalse(sender.done())
        arrived=asyncio.create_task(clock.arrive(1,{'epoch':epoch,'t':16}))
        self.assertEqual(await arrived,{'deliver':[10]});self.assertEqual(await sender,{'deliver':[]})
        await clock.network(0,{'kind':'receive','source':2,'hash':'two','id':11})
        await clock.network(1,{'kind':'send','target':1,'hash':'two','t':24})
        self.assertEqual(clock.packets,{});self.assertEqual(clock.receipts,{})
        self.assertEqual(clock.deliveries[0],[(32,11)])
        clock.close()
