require "mkmf"
dir_config('portmidi')
dir_config('porttime')
have_library('portmidi', 'Pm_Initialize')
create_makefile("portmidi")