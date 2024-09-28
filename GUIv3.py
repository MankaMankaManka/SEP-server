import tkinter as tk
import subprocess
import threading
import sys
#app is intended to start from Homev1.py

main = tk.Tk()
main.title("to client:") #this should update to tell you which clients you are talking to. Have to pass a client
main.geometry('500x700')
if len(sys.argv)==2:
    main.title(sys.argv[1])

#function to open a new home screen
def goHome():
    msg_text.config(state="normal")  # Enable writing in Text widget
    new_thread = threading.Thread(target=homeScreen, daemon=True)
    new_thread.start()
    msg_text.config(state="disabled")  # Disable Text widget again


def homeScreen():
    #run a new homepage
    subprocess.run(["python3", "Homev1.py"])

#layout for home page button
homebtn= tk.Button(text="Home", command=goHome, cursor="hand2")
homebtn.pack(side='top', expand=False, fill='both', ipady=5)

# Scrollbar
scrollbar = tk.Scrollbar(main, orient='vertical')
scrollbar.pack(side='right', fill='y')

# Text widget to hold messages
msg_text = tk.Text(main, yscrollcommand=scrollbar.set, wrap="word", bg = "grey", fg= "black", state="disabled", spacing1=7)
msg_text.pack(expand=True, fill='both')

# Entry frame for input
etry_frame = tk.Frame(main)

# Entry widget
msg_etry = tk.Entry(etry_frame)
msg_etry.insert(0, "Hello")

# Function to add messages

def enter(e): #for some reason enter key is an arguement, so we call send using this function instead
    send()

def send():
    if msg_etry.get() != "":
        msg_text.config(state="normal")  # Enable writing in Text widget
        msg_text.insert(tk.END, "YOU: " + msg_etry.get() + "\n")  # Insert message
        msg_etry.delete(0, tk.END)  # Clear entry box
        msg_text.config(state="disabled")  # Disable Text widget again
        msg_text.see(tk.END)  # Auto-scroll to the bottom

# Send button
sndbtn = tk.Button(etry_frame, text="Send", command=send, cursor="hand2")
main.bind('<Return>',enter) #send message if enter key pressed

# Scrollbar config
scrollbar.config(command=msg_text.yview)

# Layout for entry and send button
etry_frame.pack(side='bottom', fill='x')
sndbtn.pack(side='right', ipady=4)
msg_etry.pack(side='right', expand=True, fill='both', ipady=5)

main.mainloop()
