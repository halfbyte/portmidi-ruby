require "portmidi"

class TestPortmidi < Object
  include Portmidi
  def run
    s = MidiSystem.new
    di = MidiDeviceInfo.get(0)
    puts di.input?
    puts di.output?
    
    md = MidiDevice.new(0)
    loop do
      if p = md.poll
        read, msg =  md.read
        if (read>0)
          event = msg.map {|m| "%2x" % m }.join(" ")
          puts event
        end
      end
    end
    s.destroy
  end
end

TestPortmidi.new.run