require 'gserver'
require 'cli/CliInterpreter.rb'

# This class listens on CLI commands and sends them to CliInterpreter
class CliServer < GServer
    def initialize(interpreter, listenPort)
        super(listenPort)
        @interpreter = interpreter
        @audit = true
        @debug = true
    end
    
    def serve(io)
        loop do	  
            input = io.gets
	    break if !input
	    input = input.chop!
            response = @interpreter.interpret(input)
            puts "Sending response #{response}"
            io.puts(response)
        end
    end          
    
    def error(detail)
        log(detail)
        log(detail.backtrace.join("\n"))       
    end
    
#    def log(msg)
#        puts "XXXXXXXX: #{msg}"
#    end    
end

#commandParser1 = CommandParser.new("add")
#parser = CliParser.new
#parser.addCommandParser(commandParser1)
#interpreter = CliInterpreter.new(parser)
#server = CliServer.new(interpreter, 4223)
#server.start
#sleep(10000000000000)