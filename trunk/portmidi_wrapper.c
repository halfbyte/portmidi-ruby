#include "ruby.h"
#include "portmidi.h"
#include "portmidi_wrapper.h"

/*
 MidiDevice constructor. Opens the Device with the given device id
 Will open an Input stream on an input device and an Output stream on
 an output device.
*/
static VALUE md_init(VALUE self, VALUE device_number) {
	DeviceData *device;
	PmError error;
	int device_id;
	int count;
	PmStream *midiStream;
	
	device_id = NUM2INT(device_number);
  	Data_Get_Struct(self,DeviceData,device);
	device->deviceInfo = Pm_GetDeviceInfo(device_id);
	device->id = device_id;
  	return self; 
}

static VALUE md_free(void *p) {
	DeviceData *device;
	device = p;
	free(device);
}

static VALUE md_alloc(VALUE klass) {
	DeviceData *device;
	VALUE obj;
	device = malloc(sizeof(*device));
	device->stream = NULL;
	obj = Data_Wrap_Struct(klass, 0, md_free, device);
	return obj;	
}

static VALUE md_open(VALUE self) {
	DeviceData *device;
	PmError error;
	PmStream *stream;
	
	Data_Get_Struct(self,DeviceData,device);
		
	if (device->deviceInfo->input) {
		error = Pm_OpenInput(&stream, (PmDeviceID)device->id, NULL, 255, NULL, NULL);
	} else {
		error = Pm_OpenOutput(&stream, (PmDeviceID)device->id, NULL, 255, NULL, NULL, 0);
	}
	device->stream = stream;
	
	if (rb_block_given_p()) {
		rb_yield(self);
		Pm_Close(stream);
	}
	return self;
}

/* 
  Sets the midi filter for the MidiDevice
  Returns 0 or error code (if <0)

call-seq:
	filter=(filter_bitmask) -> error
	
*/
static VALUE md_set_filter(VALUE self, VALUE filter_value) {
	DeviceData *device;
	PmError error;
	long filters;
	VALUE err;
	
	Data_Get_Struct(self,DeviceData,device);
	filters = NUM2LONG(filter_value);
	error = Pm_SetFilter(device->stream, filters);
	
	err = INT2NUM(error);
	return err;
}
/* 
  Sets a channel mask for the MidiDevice
  (probably only useful on input devices)
  Returns 0 or error code (if <0)
  wants a 16bit bitfield

call-seq:
	channel_mask=(channel_bitmask) -> error
	
*/
static VALUE md_set_channel_mask(VALUE self, VALUE mask_value) {
	DeviceData *device;
	PmError error;
	int mask;
	VALUE err;

	Data_Get_Struct(self,DeviceData,device);
	mask = NUM2INT(mask_value);
	error = Pm_SetChannelMask(device->stream, mask);

	err = INT2NUM(error);
	return err;	
}

/*
	Reads an Event from the Input stream. An event consists of up to 4 bytes. The method returns 
	error,[b1,b2,b3,b4] where error is the number of bytes read or the error code (if <0)
	call-seq:
		read -> error, [b1,b2,b3,b4]
		
*/ 
static VALUE md_read(VALUE self) {
	DeviceData *device;
	VALUE msg;
	VALUE err;
	VALUE array;
	VALUE msg_ary;
	int data = 0;
	int shift = 0;
	VALUE msg_data;
	PmEvent message;
	PmError error;

	Data_Get_Struct(self,DeviceData,device);
	error = Pm_Read(device->stream, &message, 1);
	
	array = rb_ary_new2(2);
	err = INT2NUM(error);
	rb_ary_push(array, err);

	if (error>0) {
		msg_ary = rb_ary_new2(4);
		for (shift = 0; shift < 32; shift += 8) {
			data = (message.message >> shift) & 0xFF;
			msg_data = INT2NUM(data);
			rb_ary_push(msg_ary, msg_data);
		}
		rb_ary_push(array, msg_ary);
	}
	return array;
	
}
/*
Writes sysex message. The message must be a properly terminated sysex message,
otherwise very bad things may happen. returns 0 on success and error code otherwise

call-seq:
 	write_sysex(message) -> error

*/
static VALUE md_write_sysex(VALUE self, VALUE sysex) {
	DeviceData *device;
	PmError error;
	VALUE err;
	VALUE sysex_string;
	Data_Get_Struct(self,DeviceData,device);
	sysex_string = StringValue(sysex);
	
	error = Pm_WriteSysEx(device->stream, 0, (unsigned char *)RSTRING(sysex_string)->ptr);	
	err = INT2NUM(error);
	return err;	
	
}
/* 
writes a short midi message (such as note on, note off etc.). returns 0 on succes, error code otherwise
message is an array of bytes (as Fixnums) that is between 2 and 4 bytes long. The bytes are and'ed with 0xff 
before sending.

call-seq:
	write_short(message) -> error
	
*/
static VALUE md_write_short(VALUE self, VALUE bytes) {
	DeviceData *device;
	PmError error;
	VALUE err;
	int shift = 32;
	int i = 0;
	long msg = 0;
	VALUE byte_value;
	long byte = 0;
	long len;
	
	Data_Get_Struct(self,DeviceData,device);
	
	len = RARRAY(bytes)->len;
	if (len > 4) len = 4;
	
	for (i=0;i<len;i++) {
		byte_value = rb_ary_entry(bytes,i);
		byte = NUM2LONG(byte_value);
		msg = msg | (byte & 0xFF) << (i)*8;
		shift -= 8;

	}
	error = Pm_WriteShort(device->stream, 0, msg);
	err = INT2NUM(error);
	return err;
}
/*
  Returns an error message in textual form for a given error code

call-seq:
	error_text(error_code) -> message

*/
static VALUE ms_error_text(VALUE self, VALUE error_value) {
	const char *error_message;
	PmError error;
	VALUE error_text;
	
	error = NUM2INT(error_value);
	error_message = Pm_GetErrorText(error);
	error_text = rb_str_new2(error_message);
	return error_text;
}
/*
	tests for host error
	can be called at any time or after a method returns the host error error code
	if returns true, host_error_text should be called to clear the error

call-seq:
	host_error? -> boolean

*/

static VALUE md_host_error(VALUE self) {
	DeviceData *device;
	int error;
	
	Data_Get_Struct(self,DeviceData,device);
	error = Pm_HasHostError(device->stream);
	if(error) return Qtrue;
	return Qfalse;
}

/*

	returns an error message if a host error occured, returns an empty string otherwise
	TODO: eventually unify with host_error?

call-seq:
	host_error_text -> message
	
*/
static VALUE md_host_error_text(VALUE self) {
	VALUE error_text;
	char error_message[PM_HOST_ERROR_MSG_LEN];
	
	Pm_GetHostErrorText(error_message, PM_HOST_ERROR_MSG_LEN);
	error_text = rb_str_new2(error_message);
	return error_text;
}

/*
  Returns true if the input stream contains events to be fetched by read. 
  
  Returns false if no events are pending
  
  Returns error code (<0) if an error occurred

  call-seq:
	md_poll -> result
	
*/
static VALUE md_poll(VALUE self) {
	DeviceData *device;
	VALUE more;
	PmError error;
	Data_Get_Struct(self,DeviceData,device);
	error = Pm_Poll(device->stream);
	if (error == TRUE) return Qtrue;
	if (error == FALSE) return Qfalse;
	more = INT2NUM(error);
	return more;
}



/*
Returns the device name

call-seq:
	name -> name

*/

static VALUE md_name(VALUE self) {
  	VALUE string;
	DeviceData *device;
	
	Data_Get_Struct(self,DeviceData, device);		
  	string = rb_str_new2(device->deviceInfo->name);
  	return string;
}

/*
Returns the device id

call-seq:
	device_id -> id

*/
static VALUE md_device_id(VALUE self) {
	DeviceData *device;
	
	Data_Get_Struct(self,DeviceData, device);
	return INT2NUM(device->id);
}


/*
  initializes the midi system

  fires Pm_Initialize()

*/
static VALUE ms_init(VALUE self) {
	Pm_Initialize();
	return self;
}


/*
  this function should be called after the midi system has been used. Fires Pm_Terminate()
  (only needed if open is not used with block syntax)
*/
static VALUE ms_close(VALUE self) {
	Pm_Terminate();
	return self;
}


static VALUE ms_open(VALUE self) {
	
	int device_count;
	int i=0;
	VALUE inputs;
	VALUE outputs;
	VALUE self_object;
	
	self_object = rb_class_new_instance(0,NULL, cMidiSystem);
	
	Pm_Initialize();
	device_count = Pm_CountDevices();
	inputs = rb_ary_new();
	outputs = rb_ary_new();
	
	for(i=0;i<device_count;i++) {
		VALUE device_id[1];
		VALUE device_object;
		DeviceData *device;
		
		device_id[0] = INT2NUM(i);
		device_object = rb_class_new_instance(1,device_id, cMidiDevice);
		
		Data_Get_Struct(device_object,DeviceData, device);
		if (device->deviceInfo->input) {
			rb_ary_push(inputs, device_object);
		} else {
			rb_ary_push(outputs, device_object);
		}
	}
	
	rb_iv_set(self_object, "@inputs", inputs);
	rb_iv_set(self_object, "@outputs", outputs);
	
	if (rb_block_given_p()) {
		rb_ensure(&rb_yield, self_object, &ms_close, self_object);
	}
	return self_object;
	
}



static VALUE ms_inputs(VALUE self) {
	return rb_iv_get(self, "@inputs");
}
static VALUE ms_outputs(VALUE self) {
	return rb_iv_get(self, "@outputs");
}

void Init_portmidi() {
	mPortmidi = rb_define_module("Portmidi");

	// MidiSystem
	cMidiSystem = rb_define_class_under(mPortmidi, "MidiSystem", rb_cObject);
	rb_define_method(cMidiSystem, "initialize", ms_init, 0);
	rb_define_singleton_method(cMidiSystem, "open", ms_open, 0);
	rb_define_method(cMidiSystem, "close", ms_close, 0);
	rb_define_method(cMidiSystem, "inputs", ms_inputs, 0);
	rb_define_method(cMidiSystem, "outputs", ms_outputs, 0);
	rb_define_method(cMidiSystem, "error_text", ms_error_text, 1);


	// MidiDevice
	cMidiDevice = rb_define_class_under(mPortmidi, "MidiDevice", rb_cObject);
	rb_define_alloc_func(cMidiDevice, md_alloc);

  	rb_define_method(cMidiDevice, "initialize", md_init, 1);
  	rb_define_method(cMidiDevice, "open", md_open, 0);
	rb_define_method(cMidiDevice, "name", md_name, 0);
	rb_define_method(cMidiDevice, "device_id", md_device_id, 0);

  	rb_define_method(cMidiDevice, "poll", md_poll, 0);
  	rb_define_method(cMidiDevice, "read", md_read, 0);
 	rb_define_method(cMidiDevice, "write_short", md_write_short, 1);
	rb_define_method(cMidiDevice, "write_sysex", md_write_sysex, 1);
	rb_define_method(cMidiDevice, "host_error?", md_host_error, 0);
	rb_define_method(cMidiDevice, "host_error_text", md_host_error_text, 0);
	rb_define_method(cMidiDevice, "filter=", md_set_filter, 1);
	rb_define_method(cMidiDevice, "channel_mask=", md_set_channel_mask, 1);
	
	// setting constants...

	rb_define_const(cMidiSystem, "FILTER_ACTIVE_SENSING", LONG2NUM(PM_FILT_ACTIVE));
	rb_define_const(cMidiSystem, "FILTER_SYSEX", LONG2NUM(PM_FILT_SYSEX));
	rb_define_const(cMidiSystem, "FILTER_CLOCK", LONG2NUM(PM_FILT_CLOCK));
	rb_define_const(cMidiSystem, "FILTER_PLAY", LONG2NUM(PM_FILT_PLAY));
	rb_define_const(cMidiSystem, "FILTER_TICK", LONG2NUM(PM_FILT_TICK));
	rb_define_const(cMidiSystem, "FILTER_FD", LONG2NUM(PM_FILT_FD));
	rb_define_const(cMidiSystem, "FILTER_UNDEFINED", LONG2NUM(PM_FILT_UNDEFINED));
	rb_define_const(cMidiSystem, "FILTER_RESET", LONG2NUM(PM_FILT_RESET));
	rb_define_const(cMidiSystem, "FILTER_REALTIME", LONG2NUM(PM_FILT_REALTIME));
	
	rb_define_const(cMidiSystem, "FILTER_NOTE", LONG2NUM(PM_FILT_NOTE));
	
	rb_define_const(cMidiSystem, "FILTER_CHANNEL_AFTERTOUCH", LONG2NUM(PM_FILT_CHANNEL_AFTERTOUCH));	
	rb_define_const(cMidiSystem, "FILTER_POLY_AFTERTOUCH", LONG2NUM(PM_FILT_POLY_AFTERTOUCH));
	rb_define_const(cMidiSystem, "FILTER_AFTERTOUCH", LONG2NUM(PM_FILT_AFTERTOUCH));

	rb_define_const(cMidiSystem, "FILTER_PROGRAM", LONG2NUM(PM_FILT_PROGRAM));
	rb_define_const(cMidiSystem, "FILTER_CONTROL_CHANGE", LONG2NUM(PM_FILT_CONTROL));
	rb_define_const(cMidiSystem, "FILTER_PITCHBEND", LONG2NUM(PM_FILT_PITCHBEND));
	
	rb_define_const(cMidiSystem, "FILTER_MTC", LONG2NUM(PM_FILT_MTC));
	rb_define_const(cMidiSystem, "FILTER_SONG_POSITION", LONG2NUM(PM_FILT_SONG_POSITION));
	rb_define_const(cMidiSystem, "FILTER_SONG_SELECT", LONG2NUM(PM_FILT_SONG_SELECT));
	rb_define_const(cMidiSystem, "FILTER_TUNE", LONG2NUM(PM_FILT_TUNE));
	rb_define_const(cMidiSystem, "FILTER_SYSTEM_COMMON", LONG2NUM(PM_FILT_SYSTEMCOMMON));
}