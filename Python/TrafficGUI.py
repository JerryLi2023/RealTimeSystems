import tkinter as tk

root = tk.Tk()
root.title("Traffic System GUI")
root.geometry('700x500')

# Set the green background
canvas = tk.Canvas(root, width=700, height=500, bg='darkgreen', highlightthickness=0)
canvas.pack()

# Create basic background scenary
canvas.create_rectangle(0,210,700,290, fill="#3A3A3A", outline="")      # Create the main east-west road R3
canvas.create_line(0, 250, 700, 250, fill="#ddd", dash=(10, 10))          # Create dash lines for R3 road

canvas.create_rectangle(100, 0, 180, 700, fill="#3A3A3A", outline="")   # Create I1 road
canvas.create_line(140, 0, 140, 700, fill="#ddd", dash=(10, 10))          # Create dash lines for I1 road

canvas.create_rectangle(500, 0, 580, 700, fill="#3A3A3A", outline="")   # Create I2 road
canvas.create_line(540, 0, 540, 700, fill="#ddd", dash=(10, 10))          # Create dash lines for I2 road

canvas.create_rectangle(100, 210, 180, 290, fill="#3A3A3A", outline="") # clear the dashes in the junction for I1 and I2
canvas.create_rectangle(500, 210, 580, 290, fill="#3A3A3A", outline="")

canvas.create_rectangle(330, 0, 350, 700, fill="#583f37", outline="")     # Create railway X1
canvas.create_line(335, 0, 335, 700, fill="#bdbdbd", width=2)
canvas.create_line(345, 0, 345, 700, fill="#bdbdbd", width=2)

canvas.create_text(140, 255, text="I1", fill="white", font=("Arial", 14, "bold"))    # Text for intersections
canvas.create_text(540, 255, text="I2", fill="white", font=("Arial", 14, "bold"))
canvas.create_text(340, 255, text="X1", fill="white", font=("Arial", 14, "bold"))

root.mainloop()