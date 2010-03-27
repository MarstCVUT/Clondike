require 'cli/CliParser.rb'

class CliInterpreter        
    def initialize(parser)
        @parser = parser
        # Hash of string 'command name' to handler of those commands
        @handlers = {}
    end
    
    def addHandler(name, handler)
        @handlers[name] = handler
    end
    
    def interpret(commandLine)
        puts "Command line #{commandLine}"
        return "Empty input line" if ( !commandLine || commandLine.empty? )
        
        begin
          command = @parser.parse(commandLine)
        rescue ParseException => e
            return "Invalid command: #{e.text}"
        end
        
        handler = @handlers[command.name]
        return "No handler for command #{command.name}" if !handler
        
        return handler.handle(command)
    end
end