require "portmidi"

class TestPortmidi < Object
  include Portmidi
  def run
    s = MidiSystem.new
    puts MidiDeviceInfo.count
    di = MidiDeviceInfo.get(3)
    puts di.input?
    puts di.output?

    sysex_test

    s.destroy
  end
  
  def sysex_test
    mw = MidiDevice.new(2)
    mr = MidiDevice.new(1)
    
    puts "-#{mw.host_error_text}-"
    
    message = String.new
    [0xf0, 0x7e, 0x10, 0x06, 0x01, 0xf7].each do |b|
      message << b
    end
    error = mw.write_sysex(message)
    puts mw.error_text(error)
    
    loop do
      if p = mr.poll
        read, msg =  mr.read
        if (read>0)
          puts msg.inspect
        end
      end
    end
    
  end
  def write_test
    md = MidiDevice.new(2)
    md.write_short([0x91,0x33,0x2F,0])
    gets
    md.write_short([0x81,0x33,0x2F,0])
    md.write_short([0x91,0x7f,0x2F,0])
    gets
    md.write_short([0x81,0x7f,0x2F,0])
    
  end
  
  def read_loop
    mw = MidiDevice.new(2)
    mr = MidiDevice.new(0)
    loop do
      if p = mr.poll
        read, msg =  mr.read
        if (read>0)
          # puts msg.inspect
          err, sent_msg = mw.write_short(msg)
          puts "%8x" % sent_msg
        end
      end
    end
  end
  
end

TestPortmidi.new.run