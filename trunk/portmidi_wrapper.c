#include "ruby.h"
#include "portmidi.h"
#include "portmidi_wrapper.h"

static VALUE md_init(VALUE self, VALUE device_number) {
	DeviceStream *stream;
	PmError error;
	int device_id;
	int count;
	PmStream *midiStream;
	
	
	device_id = NUM2INT(device_number);
	
  	Data_Get_Struct(self,DeviceStream,stream);

	const PmDeviceInfo *deviceInfo = Pm_GetDeviceInfo(device_id);
	if(deviceInfo->input) {
		error = Pm_OpenInput(&midiStream, (PmDeviceID)device_id, NULL, 255, NULL, NULL);
		//stream->type = 0;
	} else {
		error = Pm_OpenOutput(&midiStream, (PmDeviceID)device_id, NULL, 255, NULL, NULL, 0);
		//stream->type = 1;
	}
	stream->stream = midiStream;
  	return self; 
}

static VALUE md_free(void *p) {
	DeviceStream *stream;
	stream = p;
	Pm_Close(stream->stream);
	free(stream);
}

static VALUE md_alloc(VALUE klass) {
	DeviceStream *stream;
	VALUE obj;
	stream = malloc(sizeof(*stream));
	stream->stream = NULL;
	obj = Data_Wrap_Struct(klass, 0, md_free, stream);
	return obj;	
}

static VALUE md_read(VALUE self) {
	DeviceStream *stream;
	VALUE msg;
	VALUE err;
	VALUE array;
	VALUE msg_ary;
	int data = 0;
	int shift = 0;
	VALUE msg_data;
	PmEvent message;
	PmError error;

	Data_Get_Struct(self,DeviceStream,stream);
	error = Pm_Read(stream->stream, &message, 1);
	
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

static VALUE md_poll(VALUE self) {
	DeviceStream *stream;
	VALUE more;
	PmError error;
	Data_Get_Struct(self,DeviceStream,stream);
	error = Pm_Poll(stream->stream);
	if (error == TRUE) return Qtrue;
	if (error == FALSE) return Qfalse;
	more = INT2NUM(error);
	return more;
}

static VALUE mdd_count(VALUE self) {
	int count;
	VALUE count_num;
	count = Pm_CountDevices();
	count_num = INT2NUM(count);
	return count_num;
}

static VALUE mdd_get(VALUE self, VALUE device_number) {
  PmDeviceInfo* deviceInfo;
  VALUE obj;
  int device_id;
  int device_count;

  device_count = Pm_CountDevices();
  if (device_id > device_count -1) return Qnil;

  device_id = NUM2INT(device_number);
  
  deviceInfo = Pm_GetDeviceInfo(device_id);
  obj = Data_Wrap_Struct(cMidiDeviceInfo, 0, 0, deviceInfo);
  return obj;
}
static VALUE mdd_name(VALUE self) {
  VALUE string;
  PmDeviceInfo* deviceInfo;
  Data_Get_Struct(self,PmDeviceInfo, deviceInfo);
  string = rb_str_new2(deviceInfo->name);
  return string;
}

static VALUE mdd_input(VALUE self) {
	VALUE ret;
  	PmDeviceInfo* deviceInfo;
  	Data_Get_Struct(self,PmDeviceInfo, deviceInfo);
	if (deviceInfo->input) return Qtrue;
	return Qfalse;
}
static VALUE mdd_output(VALUE self) {
	VALUE ret;
  	PmDeviceInfo* deviceInfo;
  	Data_Get_Struct(self,PmDeviceInfo, deviceInfo);
	if (deviceInfo->output) return Qtrue;
	return Qfalse;
}


static VALUE ms_init(VALUE self) {
	return self;
	Pm_Initialize();
}
static VALUE ms_destroy(VALUE self) {
	return self;
	Pm_Terminate();
}


void Init_portmidi() {
	mPortmidi = rb_define_module("Portmidi");

	// MidiSystem
	cMidiSystem = rb_define_class_under(mPortmidi, "MidiSystem", rb_cObject);
	rb_define_method(cMidiSystem, "initialize", ms_init, 0);
	rb_define_method(cMidiSystem, "destroy", ms_destroy, 0);

	// MidiDeviceInfo
	cMidiDeviceInfo = rb_define_class_under(mPortmidi,"MidiDeviceInfo", rb_cObject);
	rb_define_singleton_method(cMidiDeviceInfo, "get", mdd_get, 1);
	rb_define_singleton_method(cMidiDeviceInfo, "count", mdd_count, 0);
	rb_define_method(cMidiDeviceInfo, "name", mdd_name, 0);
	rb_define_method(cMidiDeviceInfo, "input?", mdd_input, 0);
	rb_define_method(cMidiDeviceInfo, "output?", mdd_output, 0);

	// MidiDevice
	cMidiDevice = rb_define_class_under(mPortmidi, "MidiDevice", rb_cObject);
	rb_define_alloc_func(cMidiDevice, md_alloc);
  	rb_define_method(cMidiDevice, "initialize", md_init, 1);
  	rb_define_method(cMidiDevice, "poll", md_poll, 0);
  	rb_define_method(cMidiDevice, "read", md_read, 0);

}