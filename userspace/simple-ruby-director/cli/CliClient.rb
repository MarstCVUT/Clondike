require 'socket'
require 'Util.rb'
require 'cli/SmartCommandLine.rb'

# Class that connects to CliServer and interacts with it via TCP
class CliClient
    def initialize(port)
        @port = port           
    end
    
    def connect
      @sock = TCPSocket.open("localhost",@port)        
      @commandLine = SmartCommandLine.new("Director> ", ".cmd.history", LineInterpreter.new(@sock))        
    end
    
    def run
        begin
          @commandLine.run()
        ensure
          @sock.close                
        end
#        print "Director> "
#        while (input = gets)
#            @sock.puts(input)			
#            @sock.flush			
#            reply = ""
#            while ( !reply.endsWith("\n")) 
#              reply = reply + @sock.readpartial(4096)
#            end            
#            
#            puts reply
#            print "Director> "
#        end
    end
    
    class LineInterpreter
        def initialize(sock)
            @sock = sock
        end
        
        def interpret(line)
            @sock.puts(line)			
            @sock.flush			
            reply = ""
            while ( !reply.endsWith("\n"))  do
              reply = reply + @sock.readpartial(4096)
            end            
            reply = reply.chop!
            
            return reply
        end
    end
end

client = CliClient.new(4223)
client.connect
client.run