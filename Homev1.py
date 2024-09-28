import tkinter as tk
import subprocess
import threading

# start up and restart work as intended. start up must still figure out how to connect to active clients list.
# click on a button to open a chatroom to that client. could also have a broadcast chat.



#HOW TO RUN?
#
# python3 Homev1.py





#TO DO
# #
# #
# #
# create a new group chat, with specific clients in the list. tick the buttons, then create a new group chat. select needs to be properly done/reengineered
# signal for unread messages? probably not worth but would be cool

# Create the main window
main = tk.Tk()
main.title("HOME")
main.geometry("500x700")

def restart():
    new_thread = threading.Thread(target=refresh, daemon=True)
    new_thread.start()
    main.destroy()

def refresh():
    #run a new homepage
    subprocess.run(["python3", "Homev1.py"])

#needs complete overhaul to add real clients
def start_home():
    # Create and add some random buttons to the array - fix later
    buttons=[]
    for i in range(1, 21):  # Creating 20 useless buttons
            buttons.append(tk.Button(main, text=f"Button {i}", cursor="hand2", command=lambda i=i: on_button_click(i)))# fix the value of i to real value, no clue how to track buttons in code
            buttons[-1].pack(pady=10)
            area.window_create('end', window= buttons[-1])
            area.insert('end', "\n")
    bootbtn.config(state="disabled")  # Disable Text widget again

def select_clients():
    selectFlag=1
    



    

#restart button
homebtn= tk.Button(text="restart", command=restart, cursor="hand2")
homebtn.pack(side='top', expand=False, fill='both', ipady=5)

#boot up button calls start_home
bootbtn= tk.Button(text="boot up", command=start_home, cursor="hand2")
bootbtn.pack(side='top', expand=False, fill='both', ipady=5)

#select button, rather than select, it might be easier to just have checks next to each client that allow you to delete or create gc, and then select button is instead a delete/create
selbtn= tk.Button(text="select", command=select_clients, cursor="hand2")
selbtn.pack(side='top', expand=False, fill='both', ipady=5)

# Scrollbar
fm = tk.Frame(main)
scrollbar = tk.Scrollbar(fm, orient='vertical')
scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

# scrollbar makes me want to die
area =tk.Text(fm)
area.pack(expand=True, fill='both')
area.configure(yscrollcommand= scrollbar.set)

# Scrollbar config
scrollbar.configure(command=area.yview)
fm.pack(expand=True, fill='both')
selectFlag=0

def openroom(number):
    new_thread = threading.Thread(target=room, daemon=True, args=(number,))#adjust later i got no clue what arguments are needed
    new_thread.start()

def room(number): # adjust for new arguments that will probably be added
    #run a new homepage
    print(number)
    string = str(number)#converts number to a string so that subprocess runs
    subprocess.run(["python3", "GUIv3.py", string])

# Function to be called when a button is clicked, can later be used to connect to the proper client
def on_button_click(number):
    if selectFlag==1:
        print(f"Button {number} clicked")
    else:
        openroom(number)

# Start the Tkinter main loop
main.mainloop()