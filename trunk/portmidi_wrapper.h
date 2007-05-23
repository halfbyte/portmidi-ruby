typedef struct _device {
  	PmStream* stream;
  	int id;
	const PmDeviceInfo* deviceInfo;
} DeviceData;

VALUE cMidiDevice;
VALUE cMidiSystem;
VALUE mPortmidi;
