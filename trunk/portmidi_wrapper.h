typedef struct _device {
  PmStream* stream;
  int type;
} DeviceStream;

VALUE cMidiDevice;
VALUE cMidiDeviceInfo;
VALUE cMidiSystem;
VALUE mPortmidi;
