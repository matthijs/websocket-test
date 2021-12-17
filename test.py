#!/usr/bin/python3
#
# websocket connection

import websocket

websocket.enableTrace(True)

ws = websocket.WebSocket()
ws.connect("wss://streaming.saxobank.com/sim/openapi/streamingws/connect?contextId=some-context-id")
print(ws.recv())
ws.close()
