import tkinter as tk
import PythonReceive as pr
import queue
import sys
import threading
import time
import random

BRIGHT = {"R": "#df3d3a", "Y": "#f8d433", "G": "#44a148"}
DIM = {"R": "#3a1210", "Y": "#3d3510", "G": "#10301a"}
BUTTON_COLOUR = "#616161"
BG_COLOUR = "#444444"
TRI_R = [0, 0, 0, 10, 10, 5]
TRI_L = [0, 5, 10, 0, 10, 10]
TRI_U = [0, 10, 10, 10, 5, 0]
TRI_D = [0, 0, 10, 0, 5, 10]

root = tk.Tk()
root.title("Traffic System GUI")
root.geometry('1000x500') # 700x500 is the GUI 300x500 is the buttons

# Set the green background
canvas = tk.Canvas(root, width=700, height=500, bg='darkgreen', highlightthickness=0)
canvas.pack(side="left")

trafficLights = {}  # Dict of all traffic lights
trafficLamps = {}          # Dict of the individual lamps 

# Create basic background scenary
canvas.create_rectangle(0,210,700,290, fill="#3A3A3A", outline="")      # Create the main east-west road R3
canvas.create_line(0, 250, 700, 250, fill="#ddd", dash=(10, 10))          # Create dash lines for R3 road

canvas.create_rectangle(100, 0, 180, 700, fill="#3A3A3A", outline="")   # Create I1 road
canvas.create_line(140, 0, 140, 700, fill="#ddd", dash=(10, 10))          # Create dash lines for I1 road

canvas.create_rectangle(520, 0, 600, 700, fill="#3A3A3A", outline="")   # Create I2 road
canvas.create_line(560, 0, 560, 700, fill="#ddd", dash=(10, 10))          # Create dash lines for I2 road

canvas.create_rectangle(100, 210, 180, 290, fill="#3A3A3A", outline="") # clear the dashes in the junction for I1 and I2
canvas.create_rectangle(520, 210, 600, 290, fill="#3A3A3A", outline="")

canvas.create_rectangle(340, 0, 360, 700, fill="#583f37", outline="")     # Create railway X1
canvas.create_line(345, 0, 345, 700, fill="#bdbdbd", width=2)
canvas.create_line(355, 0, 355, 700, fill="#bdbdbd", width=2)

canvas.create_text(140, 255, text="I1", fill="white", font=("Arial", 14, "bold"))    # Text for intersections
canvas.create_text(560, 255, text="I2", fill="white", font=("Arial", 14, "bold"))
canvas.create_text(375, 200, text="X1", fill="white", font=("Arial", 14, "bold"))

# Create boom gates
gate_l = canvas.create_rectangle(330, 210, 340, 250, fill="#e2351e", state="hidden")    # Create left boom gate
gate_r = canvas.create_rectangle(360, 250, 370, 290, fill="#e2351e", state="hidden")    # Right boom gate

#Train yay
train = canvas.create_rectangle(340, 225, 360, 275, fill="#4c96c0", outline="", state="hidden")

def draw_trafficLight(name, dir, x, y, horizontal =False, flipped = False):
    lightName = f"{name}_{dir}"   # Give an identifable name for each traffic light i.e I1_S
    if horizontal:
        w, h = 50, 40
    else:
        w, h = 40, 50
    
    trafficLights[lightName] = canvas.create_rectangle(x, y, x + w, y + h, fill="#111", outline="#555", width=2, tags=(lightName,))     # Make the basic traffic light outline
    lamps = []
    for i, colour in enumerate("RYG"):
        if horizontal and not flipped:                      # Check if the light is sideways or not
            cx, cy = (x + 10 + i * 15, y + 30) # Set position of light horizontally
        elif flipped and not horizontal:
            cx, cy = (x + 30, y + 40 - i * 15) # Set position of light vertically
        elif flipped and horizontal:
            cx, cy = (x + 40 - i * 15, y + 10) # Set position of light vertically
        else:
            cx, cy = (x + 10, y + 10 + i * 15) # Set position of light vertically
        lamps.append(canvas.create_oval(cx - 5, cy - 5, cx + 5, cy + 5, fill=DIM[colour], outline="", tags=(lightName,)))   # Add the dimmed lamp colours

    for i, colour in enumerate("YG"):       # Do the right hand turn lights
        if horizontal and not flipped:          # Check if the light is sideways or not
            cx, cy = (x + 20 + i * 15, y + 5) # Set position of light horizontally
            lamps.append(canvas.create_polygon(TRI_U[0]+cx,TRI_U[1]+cy,TRI_U[2]+cx,TRI_U[3]+cy,TRI_U[4]+cx,TRI_U[5]+cy, fill=DIM[colour], outline="", tags=(lightName,)))   # Add the dimmed lamp colours
        elif flipped and not horizontal:
            cx, cy = (x + 5, y + 20 - i * 15) # Set position of light vertically
            lamps.append(canvas.create_polygon(TRI_L[0]+cx,TRI_L[1]+cy,TRI_L[2]+cx,TRI_L[3]+cy,TRI_L[4]+cx,TRI_L[5]+cy, fill=DIM[colour], outline="", tags=(lightName,)))   # Add the dimmed lamp colours
        elif flipped and horizontal:
            cx, cy = (x + 20 - i * 15, y + 25) # Set position of light horizontally
            lamps.append(canvas.create_polygon(TRI_D[0]+cx,TRI_D[1]+cy,TRI_D[2]+cx,TRI_D[3]+cy,TRI_D[4]+cx,TRI_D[5]+cy, fill=DIM[colour], outline="", tags=(lightName,)))   # Add the dimmed lamp colours
        else:
            cx, cy = (x + 25, y + 20 + i * 15) # Set position of light vertically
            lamps.append(canvas.create_polygon(TRI_R[0]+cx,TRI_R[1]+cy,TRI_R[2]+cx,TRI_R[3]+cy,TRI_R[4]+cx,TRI_R[5]+cy, fill=DIM[colour], outline="", tags=(lightName,)))   # Add the dimmed lamp colours
        
    trafficLamps[lightName] = lamps
    canvas.tag_bind(lightName, "<Button-1>", lambda e, n=lightName: light_click(n, e))  # Add a button to the traffic lights

#canvas.create_polygon(TRIANGLE_POINTS, fill="white")

# Set the indiviual light in each traffic light - i.e set_trafficLamp("I1_S", "Y", "G") sets the I1 south traffic light to yellow with green right arrow 
def set_trafficLamp(name, state, turnstate="R"):                                
    for lamp, colour, temp in zip(trafficLamps[name], "RYGYG", range(5)):
        if colour == state and temp < 3:
            canvas.itemconfig(lamp, fill=BRIGHT[colour])
        elif temp < 3:
            canvas.itemconfig(lamp, fill=DIM[colour])
        elif colour == turnstate and temp > 2:
            canvas.itemconfig(lamp, fill=BRIGHT[colour])
        else:
            canvas.itemconfig(lamp, fill=DIM[colour])
        
            


# Set if the traffic gate is down or not
def set_gate(down=False):
    if down:
        canvas.itemconfig(gate_l, state="normal")       #if the gate is down show the rectangle if it is up then hide it
        canvas.itemconfig(gate_r, state="normal")
        canvas.itemconfig(train, state="normal")        # also do the train
    else:
        canvas.itemconfig(gate_l, state="hidden")
        canvas.itemconfig(gate_r, state="hidden")
        canvas.itemconfig(train, state="hidden")

# Draw I1 traffic lights
draw_trafficLight("I1","S", 50, 300) 
draw_trafficLight("I1","N", 190, 150, flipped=True) #+140 x -150y
draw_trafficLight("I1","W", 40, 160, horizontal=True, flipped=True) 
draw_trafficLight("I1","E", 190, 300, horizontal=True) #+150 x +140y

# Draw I2 traffic lights
draw_trafficLight("I2","S", 470, 300) 
draw_trafficLight("I2","N", 610, 150, flipped=True) #+140 x -150y
draw_trafficLight("I2","W", 460, 160, horizontal=True, flipped=True) 
draw_trafficLight("I2","E", 610, 300, horizontal=True) #+150 x +140y

# Set the default to all red
set_trafficLamp("I1_S", "R", "R")
set_trafficLamp("I1_N", "R", "R")
set_trafficLamp("I1_E", "R", "R")
set_trafficLamp("I1_W", "R", "R")

set_trafficLamp("I2_S", "R", "R")
set_trafficLamp("I2_N", "R", "R")
set_trafficLamp("I2_E", "R", "R")
set_trafficLamp("I2_W", "R", "R")

# Set up connection to UDP
CONNECTED = len(sys.argv) > 1
if CONNECTED:
    pr.ADDR = (sys.argv[1], 9000)       # Address of display node i.e 192.168.12.137 + port
else:
    print("Provide IP of display node")

applied = {} # set so set_trafficLamp only runs when something changeds

# convert received state into setting the traffic lights

def set_state(state):
    for name in trafficLamps:
        if name in state:
            next = (state[name], state.get(name + "_R"))
            if applied.get(name) != next:                               # If new state is different act on it
                set_trafficLamp(name, next[0], next[1])                 # Set the traffic light
                applied[name] = next                                    # Add it back to the dict
    set_gate(bool(state.get("gate_down")) or state.get("train"))        # Train stuff if train state
    update_panel(state)                                                 # update button panel

# Safing for when no display node
def safe(fn, *args):
    try:
        fn(*args)
    except OSError as e:
        print("display node unreachable:", e)

# car waiting function, S for straight, R for right turn
def car_waiting(name, turn="S"):
    if turn == "R":
        print("car waiting at", name, "(right turn)")
    else:
        print("car waiting at", name, "(straight)")
    canvas.itemconfig(trafficLights[name], outline = "cyan")    #Show that the light has been clicked
    root.after(600, lambda: canvas.itemconfig(trafficLights[name], outline="#555")) #after 600ms change the colour back to original
    if CONNECTED:
        threading.Thread(target=lambda: safe(pr.toggle_car, name, turn), daemon=True).start()   #do a network call but in another thread to prevent freezing

# handles the clicking if its straight or a turn
def light_click(name, event):
    hit = canvas.find_overlapping(event.x, event.y, event.x, event.y)   # determine location of mouse
    right_arrow = any(item in trafficLamps[name][3:] for item in hit)   # determine if the location is the right arrow
    if right_arrow:
        car_waiting(name, "R")
    else:
        car_waiting(name, "S")

# Make buttons for ped/train
def draw_button(root, text, command):
    button = tk.Label(root, text=text, bg=BUTTON_COLOUR, fg="white", padx=6, pady=5, cursor="hand2") 
    button.bind("<Button-1>", lambda e: command())  #make the button clickable
    return button

temp_train = {"on": False}  # temp dict for toggle
# toggle the train
def set_train():
    temp_train["on"] = not temp_train["on"] # flip
    on = temp_train["on"]
    if on:
        print("Train has arrived")
    else:
        print("Train has departed")
    train_button.config(text="Train has arrived" if on else "Train has departed", bg="#1aaf3b" if on else BUTTON_COLOUR)    #flip the button colour
    if CONNECTED:
        threading.Thread(target=lambda: safe(pr.set_train, int(on)), daemon=True).start()   # communicate to network

# temp ped dicts
PED_NAMES = ("Left N/S", "Right N/S", "Top E/W", "Bottom E/W")      # Name of the ped crossings
PED_NODES = {"I1": 2, "I2": 3}                                       # display-node ids of the pedestrian nodes
ped_buttons = {}                  # dict to keep track of the buttons

# Send the ped button press info to network
def press_pedestrian(name, id):
    print("pedestrian button", name, PED_NAMES[id])
    if CONNECTED:
        threading.Thread(target=lambda: safe(pr.press_button, PED_NODES[name], id), daemon=True).start()

# Change colour of button depending on ped state
def update_panel(state):
    for (name, id), button in ped_buttons.items():
        lamp = state.get(f"{name}_PED_{id}")    # id G = walk, Y = yellow, R = stop, None = offline
        next = state.get(f"{name}_BTN_{id}")
        if lamp == "G":
            bg = BRIGHT["G"]
            fg = "white"
        elif lamp == "Y":
            bg = BRIGHT["Y"]
            fg = "black"
        elif next:
            bg = "#efa300"  #Button is in queue colour i.e pending
            fg = "white"
        else:
            bg = BUTTON_COLOUR
            fg = "white"
        button.config(bg=bg, fg=fg)

# Setup the panel
panel = tk.Frame(root, bg=BG_COLOUR)
panel.pack(side="left", fill="both", expand=True)
tk.Label(panel, text="Controls", bg=BG_COLOUR, fg="white", font=("Arial", 14, "bold")).pack(pady=(12, 6))           #Title of panel

tk.Label(panel, text="Train (X1)", bg=BG_COLOUR, fg="#bbb", font=("Arial", 10, "bold")).pack(anchor="w", padx=16)    # Label of train button
train_button = draw_button(panel, "Train has arrived", set_train)                                                       # make train button
train_button.pack(fill="x", padx=16, pady=(2, 2))

for name in ("I1", "I2"):
    tk.Label(panel, text=f"{name} pedestrian buttons", bg=BG_COLOUR, fg="#bbb", font=("Arial", 10, "bold")).pack(anchor="w", padx=16)
    grid = tk.Frame(panel, bg=BG_COLOUR)
    grid.pack(fill="x", padx=16, pady=(2, 8))
    grid.columnconfigure((0, 1), weight=1, uniform="ped") 
    for id, label in enumerate(PED_NAMES):
        button = draw_button(grid, label, lambda n=name, i=id: press_pedestrian(n, i))          # draw ped button
        button.grid(row=id // 2, column=id % 2, sticky="ew", padx=2, pady=2)                    # Place button on grid
        ped_buttons[(name, id)] = button                                                        # Add button to dict

# Add function to randomly click cars

random_job = None

# Send info from random generated cars to network
def send_random_cars(rand):
    for light, turn in rand:
        safe(pr.send_car, pr.INTERSECTIONS[light[:2]][0], pr.MOVES[light[3]][turn], 1)

def random_cars():
    global random_job
    picks = [(light, random.choice("SR")) for light in random.sample(list(trafficLights), random.randint(3, 6))]    # Choose a random direction of straight or right and number of times between 3 and 6
    if CONNECTED:
        threading.Thread(target=send_random_cars, args=(picks,), daemon=True).start()   # Send the cars to the network on seperate thread
    random_job = root.after(10000, random_cars)

# Make the toggle for random cars
def toggle_random_cars():
    on = random_button.cget("bg") == BUTTON_COLOUR  # if the button is grey then it is off
    if on:
        random_button.config(bg="#1aaf3b" ) # turn green 
        random_cars()
    else:
        random_button.config(bg=BUTTON_COLOUR)        # back to grey
        root.after_cancel(random_job)       # Stop after finished


random_button = draw_button(panel, "Random cars", toggle_random_cars)
random_button.pack(fill="x", padx=16, pady=(8, 2))
    


# poll worker threads
updates = queue.Queue()

def poll_workers():
    while True:
        try:
            updates.put((pr.to_gui_state(pr.fetch_state()), True))  # get new states
        except (OSError, ValueError, KeyError):
            updates.put((pr.offline_state(), False))                # set to offline mode
        time.sleep(0.25)    # wait

def drain():
    latest = None
    while not updates.empty():
        latest = updates.get()      # get new states
    if latest:
        set_state(latest[0])      # apply new state to traffic lights
        root.title("Traffic System GUI" + ("" if latest[1] else "  -  DISPLAY NODE OFFLINE"))
    root.after(100, drain)

# start program
if CONNECTED:
    threading.Thread(target=poll_workers, daemon=True).start()
    drain()

root.mainloop()

