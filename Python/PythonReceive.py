import json
import socket

ADDR = ("192.168.12.137", 9000)               # address of display node
TIMEOUT = 0.5
RETRIES = 2                              # retry count
STALE_MS = 2000                          # node counts as offline if its last report is older than this
COLOUR = {0: "G", 1: "Y", 2: "R"}        # wire colour - 0 green, 1 yellow, 2 red

# Order of movements: NE NS NW EN ES EW SN SE SW WN WE WS i.e  NE = coming from north, going east
# S = straight, L / R = left / right
MOVES = {"N": {"S": 1, "L": 0, "R": 2}, "E": {"S": 5, "L": 4, "R": 3},
         "S": {"S": 6, "L": 8, "R": 7}, "W": {"S": 10, "L": 9, "R": 11}}
INTERSECTIONS = {"I1": (0, 2), "I2": (1, 3)}     # name -> (traffic node, pedestrian node)

_waiting = {}                                     # dict of nodes and their movements

# ping the udp socket
def _ask(text):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(TIMEOUT)
        for _ in range(RETRIES + 1):
            try:
                sock.sendto(text.encode(), ADDR)
                return sock.recvfrom(8192)[0].decode()
            except socket.timeout:
                continue
    raise TimeoutError("display node not answering")      # an OSError

#load state form json
def fetch_state():
    return json.loads(_ask("STATE"))

# send that there is a car include a count
def send_car(node, move, count=1):
    _ask(f"CAR {node} {move} {count}")

# send that a ped button has been pressed
def press_button(node, idx):
    _ask(f"BUTTON {node} {idx}")

# Send that a train has been set
def set_train(present):
    _ask(f"TRAIN {int(present)}")

# Car handling, key is name i.e I1_N, turn is S for straightm, R for right, left is included in straight
def toggle_car(key, turn="S"):
    name, approach = key.split("_")
    node, move = INTERSECTIONS[name][0], MOVES[approach][turn]
    value = 0 if _waiting.get((node, move)) else 1
    send_car(node, move, value)
    _waiting[(node, move)] = value

# return live node
def _live(node):
    return node["seen"] and node["age_ms"] <= STALE_MS

# Colour the lamp for a movement else keep lamp dark if node is not responding
def _lamp(node, idx):
    if not _live(node):
        return None
    return COLOUR[node["colour"]] if node["active"][idx] else "R"   # active[] - selects, colour - lamp

# JSON conversion to states
def to_gui_state(packet):
    nodes, out = packet["nodes"], {}
    for name, (t, p) in INTERSECTIONS.items():
        traffic, ped = nodes[t], nodes[p]
        for d, m in MOVES.items():
            out[f"{name}_{d}"] = _lamp(traffic, m["S"])
            out[f"{name}_{d}_L"] = _lamp(traffic, m["L"])
            out[f"{name}_{d}_R"] = _lamp(traffic, m["R"])
        for k in range(4):                        # Left_NS Right_NS Top_EW Bottom_EW
            out[f"{name}_PED_{k}"] = _lamp(ped, 12 + k)
            out[f"{name}_BTN_{k}"] = bool(ped["buttons"][k])
    train = nodes[4]
    out["train"] = bool(train["seen"] and train["train"] == 1)
    # Train node - active[0] = gate down
    out["gate_down"] = bool(train["seen"] and train["active"][0])
    return out

# offline state if display node isnt working
def offline_state():
    dead = {"seen": False, "age_ms": 0, "colour": 2, "active": [0] * 16, "buttons": [0] * 4, "train": 0}
    return to_gui_state({"nodes": [dict(dead) for _ in range(5)]})
