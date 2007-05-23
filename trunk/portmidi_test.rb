require "portmidi"

class TestPortmidi < Object
  include Portmidi
  
  
  def run
    MidiSystem.open do |ms|
      puts "inputs:"
      ms.inputs.each {|input| puts "#{input.device_id}: #{input.name}"}
      puts "outputs:"
      ms.outputs.each {|output| puts "#{output.device_id}: #{output.name}"}
      
      mr = ms.inputs[1].open
      mw = ms.outputs[1].open
      loop do
        if p = mr.poll
          read, msg =  mr.read
          if (read>0)
            err = mw.write_short(msg)
          end
        end
      end
      mr.close
      mw.close
    end
  end
  
  
  # def run
  #   s = MidiSystem.new
  #   puts MidiDeviceInfo.count
  #   di = MidiDeviceInfo.get(3)
  #   puts di.input?
  #   puts di.output?
  # 
  #   read_loop
  # 
  #   s.destroy
  # end
  # 
  # def sysex_test
  #   mw = MidiDevice.new(2)
  #   mr = MidiDevice.new(1)
  #   
  #   
  #   mr.filter=MidiSystem::FILTER_SYSEX;
  #   gets
  #   
  #   message = String.new
  #   [0xf0, 0x7e, 0x10, 0x06, 0x01, 0xf7].each do |b|
  #     message << b
  #   end
  #   
  #   error = mw.write_sysex(message)
  #   
  #   loop do
  #     if p = mr.poll
  #       read, msg =  mr.read
  #       if (read>0)
  #         puts msg.inspect
  #       end
  #     end
  #   end
  # 
  #   error = mw.write_sysex(message)
  #   loop do
  #     if p = mr.poll
  #       read, msg =  mr.read
  #       if (read>0)
  #         puts msg.inspect
  #       end
  #     end
  #   end
  # 
  #   
  # end
  # def write_test
  #   md = MidiDevice.new(2)
  #   md.write_short([0x91,0x33,0x2F,0])
  #   gets
  #   md.write_short([0x81,0x33,0x2F,0])
  #   md.write_short([0x91,0x7f,0x2F,0])
  #   gets
  #   md.write_short([0x81,0x7f,0x2F,0])
  #   
  # end
  # 
  # def read_loop
  #   mw = MidiDevice.new(2)
  #   mr = MidiDevice.new(0)
  #   puts "#{1 << 2}"
  #   mr.channel_mask = 1 << 0 | 1 << 1
  #   loop do
  #     if p = mr.poll
  #       read, msg =  mr.read
  #       if (read>0)
  #         err = mw.write_short(msg)
  #       end
  #     end
  #   end
  # end
  # 
end

TestPortmidi.new.run