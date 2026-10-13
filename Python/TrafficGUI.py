import tkinter as tk

BRIGHT = {"R": "#e53935", "Y": "#fdd835", "G": "#43a047"}
DIM = {"R": "#3a1210", "Y": "#3d3510", "G": "#10301a"}
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

# Create traffic light 
# def draw_trafficLight(name, dir, x, y, horizontal =False):
#     lightName = f"{name}_{dir}"   # Give an identifable name for each traffic light i.e I1_S
#     if horizontal:
#         w, h = 50, 20
#     else:
#         w, h = 20, 50
    
#     trafficLights[lightName] = canvas.create_rectangle(x, y, x + w, y + h, fill="#111", outline="#555", width=2, tags=(lightName,))     # Make the basic traffic light outline
#     lamps = []
#     for i, colour in enumerate("RYG"):
#         if horizontal:                      # Check if the light is sideways or not
#             cx, cy = (x + 10 + i * 15, y + 10) # Set position of light horizontally
#         else:
#             cx, cy = (x + 10, y + 10 + i * 15) # Set position of light vertically
            
#         lamps.append(canvas.create_oval(cx - 5, cy - 5, cx + 5, cy + 5, fill=DIM[colour], outline="", tags=(lightName,)))   # Add the dimmed lamp colours
#     trafficLamps[lightName] = lamps

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

#canvas.create_polygon(TRIANGLE_POINTS, fill="white")

# Set the indiviual light in each traffic light
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
    print(temp)
        
            


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

set_trafficLamp("I1_S", "R", "R")
set_trafficLamp("I1_N", "R", "R")
set_trafficLamp("I1_E", "R", "R")
set_trafficLamp("I1_W", "R", "R")

set_trafficLamp("I2_S", "R", "R")
set_trafficLamp("I2_N", "R", "R")
set_trafficLamp("I2_E", "R", "R")
set_trafficLamp("I2_W", "R", "R")



print(trafficLamps)
set_gate(True)

root.mainloop()