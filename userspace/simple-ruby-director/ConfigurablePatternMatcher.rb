#This mix-in adds support for loading of configuration file with reg-exps
#and using them for name matching
module ConfigurablePatternMatcher
    #Loads regexp patterns
    def loadPatterns(fileName)
        @patterns = []
        IO.foreach(fileName) { |line| @patterns << Regexp.new(line.chop!); $log.debug("Loaded pattern #{line} from #{fileName}") }
    end

    #Returns true, if the string matches any of loaded patterns
    def matchesPattern(str)
        matching = false
        @patterns.each() do |pattern|                        
            matching = true if str =~ pattern
            break if matching
        end
        matching
    end
end