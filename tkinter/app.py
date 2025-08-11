from customtkinter import *
from CTkColorPicker import *
from PIL import Image

_DEFAULT_FONT = ('Avenir', 16)
_TEMP_TIME_FONT = ('Avenir', 60, 'bold')
_MAX_CHAR = 60
_CORNER_RADIUS = 16

_DARK_GRAY = '#171717'
_OFF_WHITE = '#f0f1f2'
_AQUA = '#C0FAFF'
_DARK_NAVY_BLUE = '#132135'
_DARK_BLUE = '#2D507F'
_MEDIUM_BLUE = '#4288C5'

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
            corner_radius=_CORNER_RADIUS,
            fg_color=_DARK_GRAY,
            text_color=_OFF_WHITE,
            #border
            border_width=2,
            border_color=_OFF_WHITE
        )
        self._inp_location_text.bind('<Key>', self._text_limit)
        self._inp_location_text.bind('<Return>', self._on_enter)

        #input submit button
        self._inp_button = self._make_button(
            text="Continue",
            command=self._on_button_press,
            master=self._inp_frame
        )

    def _text_limit(self, event):
        text = event.widget.get()
        if len(text) >= _MAX_CHAR and event.keysym != 'BackSpace':
            return 'break'

    def _on_enter(self, event):
        return 'break'

    def _on_button_press(self):
        self._inp_button.focus_set()
        location = self._inp_location_text.get() #WIP

        self._window.geometry(f'{_WINDOW_WIDTH}x{_WINDOW_HEIGHT_FINAL}')

        #rgb panel
        self._load_rgb()

        #tab to switch b/tw time & clock
        self._load_time_clock()

        self._reveal_final()

    def _load_rgb(self):
        self._rgb_frame = self._make_frame(200,300)
        self._color_label = CTkLabel(
            master=self._rgb_frame,
            text='Display Color:',
            font=_DEFAULT_FONT,
            text_color=_OFF_WHITE
        )
        self._color_picker = CTkColorPicker(
            master=self._rgb_frame,
            orientation=HORIZONTAL,
            width=250,
            corner_radius=_CORNER_RADIUS,
            fg_color=_DARK_GRAY,
            button_color=_MEDIUM_BLUE,
            button_hover_color=_DARK_BLUE
        )
        self._color_button = self._make_button(
            text="Set Color",
            master=self._rgb_frame,
            width=250,
            command=lambda: print("Selected color:", self._color_picker.get())  #WIP
        )

    def _load_time_clock(self):
        self._data_tab = CTkTabview(
            master=self._window,
            width=300,
            anchor='w',
            fg_color=_DARK_NAVY_BLUE,
            text_color=_OFF_WHITE,
            segmented_button_fg_color=_OFF_WHITE,
            segmented_button_selected_color=_DARK_BLUE,
            segmented_button_unselected_color=_DARK_GRAY,
            segmented_button_selected_hover_color=_DARK_BLUE,
            segmented_button_unselected_hover_color=_MEDIUM_BLUE,
            #border
            corner_radius=_CORNER_RADIUS,
            border_width=2,
            border_color=_OFF_WHITE,
        )
        #clock tab
        self._clock_tab = self._data_tab.add('Time')

        #weather tab
        self._weather_tab = self._data_tab.add('Weather')
        self._degree_switch = CTkSwitch(
            master=self._weather_tab,
            text='Celsius/Fahrenheit',
            font=_DEFAULT_FONT,
            text_color=_OFF_WHITE
        )
        self._feels_temp_switch = CTkSwitch(
            master=self._weather_tab,
            text='Feels-Like/Air Temp',
            font=_DEFAULT_FONT,
            text_color=_OFF_WHITE
        )
        cloud_img = Image.open('tkinter/icons/cloud.png')
        weather_icon = CTkImage(light_image=cloud_img, size=(64, 60))
        self._weather_icon_label = CTkLabel(
            master=self._weather_tab,
            image=weather_icon,
            text=''
        )
        self._weather_temp_label = CTkLabel(
            master=self._weather_tab,
            font=_TEMP_TIME_FONT,
            text='00°',
            text_color=_OFF_WHITE
        )
        self._feels_temp_label = CTkLabel(
            master=self._weather_tab,
            font=_DEFAULT_FONT,
            text='Feels Like: 00°',
            text_color=_OFF_WHITE,
        )
        self._weather_location_label = CTkLabel(
            master=self._weather_tab,
            font=_DEFAULT_FONT,
            text='In PLACEHOLDER',
            text_color=_OFF_WHITE
        )
        self._weather_humidity_label = CTkLabel(
            master=self._weather_tab,
            font=_DEFAULT_FONT,
            text='Humidity: 0%',
            text_color=_OFF_WHITE
        )
        self._weather_wind_label = CTkLabel(
            master=self._weather_tab,
            font=_DEFAULT_FONT,
            text='Wind Speed: 0 mph',
            text_color=_OFF_WHITE
        )

    def _make_frame(self, width, height) -> CTkFrame:
        frame = CTkFrame(
            master=self._window,
            width=width,
            height=height,
            fg_color=_DARK_NAVY_BLUE,
            corner_radius=_CORNER_RADIUS,
            border_width=2,
            border_color=_OFF_WHITE
        )
        frame.grid_propagate(False)  # Prevent frame from resizing to fit children
        return frame
    
    def _make_button(self, master, text: str, command=None, width=40, height=30) -> CTkButton:
        button = CTkButton(
        master=master,
        text=text,
        command=command,
        font=_DEFAULT_FONT,
        width=width,
        height=height,
        fg_color=_DARK_GRAY,
        text_color=_AQUA,
        hover_color=_DARK_BLUE,
        corner_radius=_CORNER_RADIUS,
        border_width=2,
        border_color=_OFF_WHITE
        )
        return button

    def _reveal_final(self):
        #RGB display
        self._rgb_frame.columnconfigure(0, weight=1)
        self._rgb_frame.rowconfigure(0, weight=1)
        self._rgb_frame.rowconfigure(1, weight=3)
        self._rgb_frame.rowconfigure(2, weight=1)

        self._rgb_frame.grid(row=1, column=0, padx=(10,0), pady=10, sticky='w' + 'n' + 's')
        self._color_label.grid(row=0, column=0, padx=10, pady=(5,0), sticky='w' + 'n')
        self._color_picker.grid(row=1, column=0, padx=10, pady=(0,5), sticky='w' + 'e' + 'n' + 's')
        self._color_button.grid(row=2, column=0, padx=10, pady=(0,10), sticky='s')

        #TAB display
        self._data_tab.grid(row=1, column=1, padx=10, pady=(0,10), columnspan=1, sticky='w' + 'e' + 'n' + 's')

        #weather tab display
        self._weather_tab.columnconfigure(0, weight=1)
        self._weather_tab.rowconfigure(0, weight=1)
        self._weather_tab.rowconfigure(1, weight=1)
        self._weather_tab.rowconfigure(2, weight=10)

        self._weather_icon_label.grid(row=0, column=0, padx=15, pady=(20,0), sticky='w' + 'n')
        self._weather_temp_label.grid(row=0, column=0, padx=90, sticky='w' + 'n')
        self._feels_temp_label.grid(row=0, column=0, padx=90, pady=(63,0), sticky='w' + 'n')
        self._weather_wind_label.grid(row=0, column=0, padx=25, pady=(25,0),sticky='e' + 'n')
        self._weather_humidity_label.grid(row=0, column=0, padx=48, pady=(25,0), sticky='e')

        self._weather_location_label.grid(row=1, column=0, sticky='w' + 'n')
        self._degree_switch.grid(row=1, column=0, pady=(20,0), sticky='w' + 's')
        self._feels_temp_switch.grid(row=1, column=0, pady=(20,0), sticky='e' + 's')

    def run(self):
        #window grid configure
        self._window.columnconfigure(0, weight=1)
        self._window.rowconfigure(0, weight=1)
        self._window.rowconfigure(1, weight=1000)
        self._window.columnconfigure(1, weight=1000)

        #frame entry configure
        self._inp_frame.columnconfigure(0, weight=100)
        self._inp_frame.columnconfigure(1, weight=1)
        self._inp_frame.rowconfigure(0, weight=1)
        self._inp_frame.grid(row=0, column=0, columnspan=2, padx=10, pady=(10,0), sticky='n')

        #entry grid
        self._inp_location_text.grid(row=0, column=0, padx=(10,3.5), sticky='w' + 'e')

        #button grid
        self._inp_button.grid(row=0, column=1, padx=(3.5,10), sticky='w' + 'e')

        #start event loop
        self._window.mainloop()

if __name__ == '__main__':
    UI = ClockUI()
    UI.run()