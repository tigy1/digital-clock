from customtkinter import *
from CTkColorPicker import *
from PIL import Image

import app_logic

_DEFAULT_FONT = ('Avenir', 16)
_TEMP_TIME_FONT = ('Avenir', 60)
_MAX_CHAR = 60
_CORNER_RADIUS = 16

_DARK_GRAY = '#171717'
_OFF_WHITE = '#f0f1f2'
_AQUA = '#C0FAFF'
_DARK_NAVY_BLUE = '#132135'
_DARK_BLUE = '#2D507F'
_DARK_RED = '#7F2D2D'
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
            command=self._on_location_button_press,
            master=self._inp_frame
        )

    def _text_limit(self, event):
        text = event.widget.get()
        if len(text) >= _MAX_CHAR and event.keysym != 'BackSpace':
            return 'break'

    def _on_enter(self, event):
        return 'break'

    def _on_location_button_press(self) -> None:
        self._inp_button.focus_set()
        location = self._inp_location_text.get() #WIP

        self._window.geometry(f'{_WINDOW_WIDTH}x{_WINDOW_HEIGHT_FINAL}')

        #rgb panel
        self._load_rgb()

        #tab to switch b/tw time & clock
        self._load_time_clock()

        self._reveal_final()

    def _on_time_format_switch(self) -> None:
        hour, minute = app_logic.parse_time(self._time_label.cget('text'))
        if self._time_format_switch.get() == 1:
            meridiem = 'AM'
            if hour > 12:
                meridiem = 'PM'
                hour %= 12
            self._time_label.configure(text=f'{hour:02d}:{minute:02d} {meridiem}')
        else:
            self._time_label.configure(text=f'{hour:02d}:{minute:02d}')
        self._time_format_switch.configure(state="disabled")
        self._time_format_switch.after(1000, lambda: self._time_format_switch.configure(state="normal"))

    def _on_hours_option(self, value: str):
        placeholder = int(value)
        original_time = self._time_label.cget('text')
        if ' ' in original_time:
            if placeholder > 12:
                placeholder %= 12
                original_time = original_time.replace('AM', 'PM')
            else:
                original_time = original_time.replace('PM', 'AM')
        
        adjusted_time = f'{placeholder:02d}' + original_time[2:]
        self._time_label.configure(text=adjusted_time)

    def _on_minutes_option(self, value):
        placeholder = int(value)
        original_time = self._time_label.cget('text')
        adjusted_time = original_time[:3] + f'{placeholder:02d}' + original_time[5:]
        self._time_label.configure(text=adjusted_time)

    def _on_degree_switch(self) -> None:
        degree_index = self._weather_temp_label.cget('text').index('°')
        weather_temp = int(self._weather_temp_label.cget('text')[:degree_index])
        parsed_feels = self._feels_temp_label.cget("text")[len('Feels Like: '):]
        feels_value = int(parsed_feels[:degree_index])
        if self._degree_switch.get() == 1:
            new_weather = int(app_logic.celsius_to_fahrenheit(weather_temp))
            self._weather_temp_label.configure(text=f'{new_weather}°F')
            new_feels = int(app_logic.celsius_to_fahrenheit(feels_value))
            self._feels_temp_label.configure(text=f'Feels Like: {new_feels}°F')
        else:
            new_weather = int(app_logic.fahrenheit_to_celsius(weather_temp))
            self._weather_temp_label.configure(text=f'{new_weather}°C')
            new_feels = int(app_logic.fahrenheit_to_celsius(feels_value))
            self._feels_temp_label.configure(text=f'Feels Like: {new_feels}°C')
        #disable for 1 second
        self._degree_switch.configure(state="disabled")
        self._degree_switch.after(1000, lambda: self._degree_switch.configure(state="normal"))

    def _on_feels_air_switch(self):
        #WIP TO ONLY PERSONALLY INTERFACE W/ DIGITAL CLOCK
        self._feels_temp_switch.configure(state="disabled")
        self._feels_temp_switch.after(1000, lambda: self._feels_temp_switch.configure(state="normal"))

    def _on_update_time_button_press(self):
        print("Updating!")
        pass #sends displayed time to physical clock

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
        self._time_tab = self._data_tab.add('Time')
        self._time_format_switch = CTkSwitch(
            master=self._time_tab,
            corner_radius=_CORNER_RADIUS,
            text='Military Time/Standard Time',
            font=_DEFAULT_FONT,
            text_color=_OFF_WHITE,
            fg_color=_DARK_BLUE,
            progress_color=_DARK_RED,
            command=self._on_time_format_switch,
            #border
            border_color=_OFF_WHITE,
            border_width=2
        )
        clock_img = Image.open('tkinter/icons/clock_icon.png')
        _clock_icon = CTkImage(light_image=clock_img, size=(64, 60))
        self._clock_icon_label = CTkLabel(
            master=self._time_tab,
            image=_clock_icon,
            text=''
        )
        self._time_label = CTkLabel(
            master=self._time_tab,
            font=_TEMP_TIME_FONT,
            text='00:00',
            text_color=_OFF_WHITE
        )
        hour_list = [f"{h:02d}" for h in range(24)]
        self._hour_option = CTkOptionMenu(
            master=self._time_tab,
            font=_DEFAULT_FONT,
            text_color=_OFF_WHITE,
            values=hour_list,
            command=self._on_hours_option
        )
        self._hour_label = CTkLabel(
            master=self._time_tab,
            font=_DEFAULT_FONT,
            text='Configure Hour',
            text_color=_OFF_WHITE,
        )
        minute_list = [f"{m:02d}" for m in range(60)]
        self._minute_option = CTkOptionMenu(
            master=self._time_tab,
            font=_DEFAULT_FONT,
            text_color=_OFF_WHITE,
            values=minute_list,
            command=self._on_minutes_option
        )
        self._minute_label = CTkLabel(
            master=self._time_tab,
            font=_DEFAULT_FONT,
            text='Configure Minutes',
            text_color=_OFF_WHITE
        )
        self._update_time_button = self._make_button(
            master=self._time_tab, 
            text='Update Time', 
            command=self._on_update_time_button_press
        )
    
        #weather tab
        self._weather_tab = self._data_tab.add('Weather')
        self._degree_switch = CTkSwitch(
            master=self._weather_tab,
            corner_radius=_CORNER_RADIUS,
            text='Celsius/Fahrenheit',
            font=_DEFAULT_FONT,
            text_color=_OFF_WHITE,
            fg_color=_DARK_BLUE,
            progress_color=_DARK_RED,
            command=self._on_degree_switch,
            #border
            border_color=_OFF_WHITE,
            border_width=2
        )
        self._feels_temp_switch = CTkSwitch( 
            master=self._weather_tab,
            text='Feels-Like Temp/Air Temp',
            font=_DEFAULT_FONT,
            text_color=_OFF_WHITE,
            fg_color=_DARK_BLUE,
            corner_radius=_CORNER_RADIUS,
            command=self._on_feels_air_switch,
            #border
            border_color=_OFF_WHITE,
            border_width=2
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
            text='00°C',
            text_color=_OFF_WHITE
        )
        self._feels_temp_label = CTkLabel(
            master=self._weather_tab,
            font=_DEFAULT_FONT,
            text='Feels Like: 00°C',
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

        self._rgb_frame.grid(row=1, column=0, padx=(10,0), pady=10, sticky='w' + 'n' + 's')
        self._color_label.grid(row=0, column=0, padx=10, pady=(5,0), sticky='w' + 'n')
        self._color_picker.grid(row=1, column=0, padx=10, pady=(0,5), sticky='w' + 'e' + 'n' + 's')
        self._color_button.grid(row=2, column=0, padx=10, pady=(0,10), sticky='s')

        #TAB display
        self._data_tab.grid(row=1, column=1, padx=10, pady=(0,10), columnspan=1, sticky='w' + 'e' + 'n' + 's')

        #clock tab display
        self._time_tab.columnconfigure(0, weight=1)
        self._time_tab.rowconfigure(0, weight=1)
        self._time_tab.rowconfigure(1, weight=1)
        self._time_tab.rowconfigure(2, weight = 10)

        self._clock_icon_label.grid(row=0, column=0, pady=(20,0), sticky='w' + 'n')
        self._time_label.grid(row=0, column=0, padx=(75,0), pady=(10,0), sticky='w' + 'n')

        self._time_format_switch.grid(row=1, column=0, sticky='w' + 'n')

        self._hour_option.grid(row=2, column=0, pady=(40,0), sticky='w' + 'n')
        self._hour_label.grid(row=2, column=0, pady=(5,0), sticky='w' + 'n')
        self._minute_option.grid(row=2, column=0, pady=(40,0), sticky='e' + 'n')
        self._minute_label.grid(row=2, column=0, padx=15, pady=(5,0), sticky='e' + 'n')
        self._update_time_button.grid(row=2, column=0, sticky='w' + 'e' + 's')

        #weather tab display
        self._weather_tab.columnconfigure(0, weight=1)
        self._weather_tab.rowconfigure(0, weight=1)
        self._weather_tab.rowconfigure(1, weight=1)
        self._weather_tab.rowconfigure(2, weight=10)

        self._weather_icon_label.grid(row=0, column=0, pady=(20,0), sticky='w' + 'n')
        self._weather_temp_label.grid(row=0, column=0, padx=75, sticky='w' + 'n')
        self._feels_temp_label.grid(row=0, column=0, padx=75, pady=(63,0), sticky='w' + 'n')
        self._weather_wind_label.grid(row=0, column=0, padx=5, pady=(25,0), sticky='e' + 'n')
        self._weather_humidity_label.grid(row=0, column=0, padx=28, pady=(25,0), sticky='e')

        self._weather_location_label.grid(row=1, column=0, pady=(15, 0), sticky='w' + 'n')
        self._degree_switch.grid(row=1, column=0, pady=(65,0), sticky='w' + 'n')
        self._feels_temp_switch.grid(row=1, column=0, pady=(110,0), sticky='w')

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