from customtkinter import *

_DEFAULT_FONT = ('Avenir', 16)
_MAX_CHAR = 60

_DARK_GRAY = '#171717'
_OFF_WHITE = '#f0f1f2'
_AQUA = '#C0FAFF'
_DARK_NAVY_BLUE = '#132135'
_DARK_BLUE = '#2D507F'

_WINDOW_WIDTH = 630
_WINDOW_HEIGHT_INITIAL = 80
_WINDOW_HEIGHT_FINAL = 400

class ClockUI:
    def __init__(self):
        self._window = CTk(fg_color=_DARK_GRAY)
        screen_width = self._window.winfo_screenwidth()
        self._window.geometry(f'{_WINDOW_WIDTH}x{_WINDOW_HEIGHT_INITIAL}+{screen_width-_WINDOW_WIDTH}+0')
        self._window.title('Clock Config')
        self._window.resizable(False, False)

        #input frame
        self._inp_frame = self._make_frame(610, 60)

        #input location entry
        self._inp_location_text = CTkEntry(
            master=self._inp_frame, 
            height=30,
            placeholder_text='Search Location',
            font=_DEFAULT_FONT,
            corner_radius=16,
            fg_color=_DARK_GRAY,
            text_color=_OFF_WHITE,
            #border
            border_width=2,
            border_color=_OFF_WHITE
        )
        self._inp_location_text.bind('<Key>', self._text_limit)
        self._inp_location_text.bind('<Return>', self._on_enter)

        #input submit button
        self._inp_button = CTkButton(
            master=self._inp_frame, 
            width=40,height=30,
            text='Continue',
            font=_DEFAULT_FONT,
            fg_color=_DARK_GRAY,
            text_color=_AQUA,
            hover_color=_DARK_BLUE,
            command=self._on_button_press,
            #border
            corner_radius=16,
            border_width=2,
            border_color=_OFF_WHITE
        )

    def _text_limit(self, event):
        text = event.widget.get()
        if len(text) >= _MAX_CHAR and event.keysym != 'BackSpace':
            return 'break'

    def _on_enter(self, event):
        return 'break'

    def _on_button_press(self):
        self._inp_button.focus_set()
        location = self._inp_location_text.get() #work on

        self._window.geometry(f'{_WINDOW_WIDTH}x{_WINDOW_HEIGHT_FINAL}')
        self._window.rowconfigure(1, weight=1000)

        #rgb panel
        self._load_rgb()

        #tab to switch b/tw time & clock
        self._load_time_clock()

        self._reveal_final()
        
    def _make_frame(self, width, height) -> CTkFrame:
        frame = CTkFrame(
            master=self._window,
            width=width,
            height=height,
            fg_color=_DARK_NAVY_BLUE,
            corner_radius=16,
            border_width=2,
            border_color=_OFF_WHITE
        )
        frame.grid_propagate(False)  # Prevent frame from resizing to fit children
        return frame

    def _load_rgb(self):
        self._rgb_frame = self._make_frame(200,300)

    def _load_time_clock(self):
        self._data_tab = CTkTabview(
            master=self._window,
            width=300, height=200
        )
        self._data_tab.add('Time')
        self._data_tab.add('Weather')

    def _reveal_final(self):
        #RGB display
        self._rgb_frame.grid(row=1, column=0, padx=10, sticky='w' + 'n' + 's')

        #TAB display
        self._data_tab.grid(row=1, column=0, padx=10, pady=10, sticky='e' + 'n' + 's')

    def run(self):
        #window grid configure
        self._window.columnconfigure(0, weight=1)
        self._window.rowconfigure(0, weight=1)

        #frame grid configure
        self._inp_frame.columnconfigure(0, weight=100)
        self._inp_frame.columnconfigure(1, weight=1)
        self._inp_frame.rowconfigure(0, weight=1)
        self._inp_frame.grid(row=0, column=0, padx=10, pady= 10, sticky='n')

        #entry grid
        self._inp_location_text.grid(row=0, column=0, padx=(10,3.5), sticky='w' + 'e')

        #button grid
        self._inp_button.grid(row=0, column=1, padx=(3.5,10), sticky='w' + 'e')

        #start event loop
        self._window.mainloop()

if __name__ == '__main__':
    UI = ClockUI()
    UI.run()