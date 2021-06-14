from datetime import datetime
import json
import random
import time
import uuid

import asyncio
import websockets

devices = {
    "ba:b3:bb:c2:dc:43": {
        "device_id": "ba:b3:bb:c2:dc:43",
        "type": "CAR-PARK",
        "online": False,
        "last_seen": "2021-05-19T16:00:47Z"
    },
    "5e:c4:be:7f:cc:61": {
        "device_id": "5e:c4:be:7f:cc:61",
        "type": "CAR-PARK",
        "online": True,
        "last_seen": "2021-05-19T17:22:51Z"
    },
    "34:6d:4e:d0:a9:e2": {
        "device_id": "34:6d:4e:d0:a9:e2",
        "type": "CAR-PARK",
        "online": False,
        "last_seen": "2021-05-19T14:09:24Z"
    },
    "52:3b:a0:58:0a:d3": {
        "device_id": "52:3b:a0:58:0a:d3",
        "type": "CAR-PARK",
        "online": False,
        "last_seen": "2021-05-19T09:58:11Z"
    },
}

cards = {
    "bab3bbc2": {
        "card_id": "bab3bbc2",
        "is_master": True,
        "registered_at": "2021-06-06T11:32:33Z",
    },
    "5ec4be7f": {
        "card_id": "5ec4be7f",
        "is_master": False,
        "registered_at": "2021-06-07T09:16:41Z",
    },
    "346d4ed0": {
        "card_id": "346d4ed0",
        "is_master": False,
        "registered_at": "2021-06-07T06:01:23Z",
    },
}

events = {
    'WS_INITIAL': 'ws:boot',
    'WS_DEVICE_CONNECTED': 'device:connected',
    'WS_DEVICE_DEAD': 'device:dead',

    'WS_CARD_AUTHORIZED': 'card:authorized',
    'WS_CARD_REMOVED': 'card:removed',

    'WS_CARPARK_UPDATE': 'carpark:update',
    'WS_ALARM_UPDATE': 'alarm:update',
    'WS_INTRUSION_UPDATE': 'intrusion:update',
    'WS_ROOF_UPDATE': 'roof:update',
    'WS_GATE_UPDATE': 'gate:update',
}

async def server(ws, path):
    payload = {
        'event': events['WS_INITIAL'],
        'lights': 0,
        'alarm': 0,
        'intrusion': 0,
        'roof': 0,
        'gate': 0,
        'devices': {},
        'cards': {},
        'slots': {}
    }

    await ws.send(json.dumps(payload))

    while True:
        kind = random.randrange(0, 2)
        wait = random.randrange(2, 10)
        print(f"Waiting {wait} seconds for the next {kind} device")
        await asyncio.sleep(wait)

        if kind == 0:
            payload = { 'event': events['WS_DEVICE_CONNECTED'], 'device_id': str(uuid.uuid4()), 'type': 'CAR-PARK', 'online': bool(random.randint(0, 1)), 'last_seen': datetime.now().strftime('%Y-%m-%dT%H:%M:%SZ')}
            await ws.send(json.dumps(payload))
        elif kind == 1:
            payload = { 'event': events['WS_CARD_AUTHORIZED'], 'card_id': str(uuid.uuid4()).replace(':', '')[:8], 'is_master': bool(random.randint(0, 100) < 20), 'registered_at': datetime.now().strftime('%Y-%m-%dT%H:%M:%SZ')}
            await ws.send(json.dumps(payload))

start_server = websockets.serve(server, "localhost", 6789)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
